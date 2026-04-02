#pragma once

#include <array>

#include "BalancerMath.h"
#include "LookAheadDelay.h"
#include "LoudnessEstimator.h"
#include "PluginParameters.h"
#include "SpectralDuckEQ.h"
#include "TrackProfile.h"
#include "VisualizationState.h"

namespace liveflow
{
/**
 * @class BalancerEngine
 * @brief Core Audio Engine responsible for the dual-signal balancing and volume ducking/expansion logic.
 *
 * This class operates entirely on the strict real-time audio thread. It guarantees zero dynamic 
 * memory allocation (malloc/new) and zero blocking locks. All inter-thread communication to the UI 
 * is done through standard Atomics pushed via `VisualizationState`.
 *
 * Features:
 * - Real-time ITU-R BS.1770 LUFS analysis (K-Weighting).
 * - Exponential Moving Average (EMA) anchors for Auto-Tracking modes.
 * - Hardware exact `LookAheadDelay` circular buffering to pre-emptively act on incoming transient signals.
 *
 * @tparam SampleType The audio float format type (e.g., float or double).
 */
template <typename SampleType>
class BalancerEngine
{
public:
    /**
     * @brief Allocates pre-flight buffers and prepares all DSP algorithms for processing.
     * 
     * Handles establishing ring buffer sizes based on the hardware host's defined sample rate
     * and max allowed block size limits. 
     */
    void prepare (const double newSampleRate,
                  const int maximumBlockSize,
                  const int mainChannels,
                  const int sidechainChannels)
    {
        sampleRate = newSampleRate;
        mainChannelCount = juce::jlimit (1, 2, mainChannels);
        sidechainChannelCount = juce::jlimit (1, 2, sidechainChannels);

        // Allocate lookahead delay up to a max hard cap of 50ms capacity to prevent buffer overflow.
        const auto maxDelaySamples = static_cast<int> (std::ceil (sampleRate * 0.05));
        lookAheadDelay.prepare (mainChannelCount, maxDelaySamples);
        mainLoudness.prepare (sampleRate, mainChannelCount);
        sidechainLoudness.prepare (sampleRate, sidechainChannelCount);
        spectralDuckEQ.prepare (sampleRate, mainChannelCount);

        // GUI drawing should run around 90 FPS max
        visualUpdateInterval = juce::jmax (24, static_cast<int> (sampleRate / 90.0));
        visualUpdateCounter = visualUpdateInterval;
        
        targetGainDb = 0.0f;
        smoothedGainDb = 0.0f;
        presenceEnvelope = 0.0f;
        heldVocalLevel = -60.0f;
        autoAnchorPoint = -24.0f;
        latencySamples = 0;
        
        juce::ignoreUnused (maximumBlockSize);
    }

    /**
     * @brief Instantly flushes all internal audio memory, delay lines, and resets smoothing functions back to idle states.
     */
    void reset (VisualizationState* visualState = nullptr) noexcept
    {
        lookAheadDelay.reset();
        mainLoudness.reset();
        sidechainLoudness.reset();
        spectralDuckEQ.reset();

        targetGainDb = 0.0f;
        smoothedGainDb = 0.0f;
        presenceEnvelope = 0.0f;
        heldVocalLevel = -60.0f;
        autoAnchorPoint = -24.0f;
        visualUpdateCounter = visualUpdateInterval;

        if (visualState != nullptr)
            visualState->reset();
    }

    /**
     * @brief Atomically accepts the latest user settings changes piped over from PluginParameters / Host UI.
     * Re-computes time constant half-life math into fast mathematical multipliers immediately.
     */
    void updateSettings (const RuntimeSettings& nextSettings) noexcept
    {
        settings = nextSettings;

        attackCoefficient = math::timeMsToCoefficient (sampleRate, juce::jmax (15.0f, settings.speedMs * 0.55f));
        releaseCoefficient = math::timeMsToCoefficient (sampleRate, juce::jmax (25.0f, settings.speedMs * 1.35f));

        // Presence detection: insanely fast attack for transient capture, user-controlled release
        presenceAttackCoefficient = math::timeMsToCoefficient (sampleRate, 2.0f);
        presenceReleaseCoefficient = math::timeMsToCoefficient (sampleRate, settings.presenceReleaseMs);

        // HeldVocalLevel tracking
        heldTrackingCoefficient = math::timeMsToCoefficient (sampleRate, 50.0f);
        heldReleaseCoefficient = math::timeMsToCoefficient (sampleRate, 1200.0f);

        // Auto anchor: extremely massive slow EMA (~10 seconds) to find the long-term stable singing point
        autoAnchorCoefficient = math::timeMsToCoefficient (sampleRate, 10000.0f);

        // Explicitly truncate look-ahead setting to hardware delay line cap of 50ms preventing boundary fault crashes
        const auto safeLookAheadMs = juce::jmin (50.0f, settings.lookAheadMs);
        latencySamples = static_cast<int> (std::round (sampleRate * safeLookAheadMs * 0.001));
        lookAheadDelay.setDelaySamples (latencySamples);
    }

    /**
     * @brief Process an N-sample block of audio through the balancing math pipeline.
     * 
     * @param mainBuffer Modifiable referenced Audio Buffer holding the backing track (will be mutated in-place)
     * @param sidechainBuffer Read-only Host-provided Sidechain Buffer (e.g., vocal track)
     * @param visualState Thread-safe Lock-Free sink to dispatch diagnostic state updates to the UI
     */
    void process (juce::AudioBuffer<SampleType>& mainBuffer,
                  const juce::AudioBuffer<SampleType>* sidechainBuffer,
                  VisualizationState& visualState) noexcept
    {
        const auto numSamples = mainBuffer.getNumSamples();
        const auto sidechainAvailable = sidechainBuffer != nullptr && sidechainBuffer->getNumChannels() > 0;

        std::array<SampleType, 2> mainFrame {};
        std::array<SampleType, 2> sidechainFrame {};

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Gather synchronized incoming sample matrices across all independent channels
            for (int channel = 0; channel < mainChannelCount; ++channel)
                mainFrame[static_cast<size_t> (channel)] = mainBuffer.getReadPointer (channel)[sample];

            for (int channel = 0; channel < sidechainChannelCount; ++channel)
            {
                sidechainFrame[static_cast<size_t> (channel)] = sidechainAvailable && channel < sidechainBuffer->getNumChannels()
                    ? sidechainBuffer->getReadPointer (channel)[sample]
                    : static_cast<SampleType> (0);
            }

            // ═══════════════════════════════════════════
            // Stage 1: Dual-signal separation & Logic detection
            // ═══════════════════════════════════════════
            
            // Substage A: Fast Envelope & Presence validation gate (detect if vocalist is physically singing)
            auto sidechainMagnitude = static_cast<SampleType> (0);
            for (int channel = 0; channel < sidechainChannelCount; ++channel)
                sidechainMagnitude += std::abs (sidechainFrame[static_cast<size_t> (channel)]);
            sidechainMagnitude /= static_cast<SampleType> (juce::jmax (1, sidechainChannelCount));

            const auto presenceCoeff = static_cast<float> (sidechainMagnitude) > presenceEnvelope
                ? presenceAttackCoefficient
                : presenceReleaseCoefficient;
            presenceEnvelope = math::smoothTowards (presenceEnvelope, static_cast<float> (sidechainMagnitude), presenceCoeff);
            const auto presenceDb = juce::Decibels::gainToDecibels (presenceEnvelope, -100.0f);
            const auto voicePresence = math::computeVoicePresence (presenceDb, settings.noiseGateDb); // Returns 0.0 ~ 1.0 sigmoid

            // Substage B: Pass raw waveforms into deep LUFS estimators (ITU-R 1770)
            mainLoudness.pushFrame (mainFrame);
            sidechainLoudness.pushFrame (sidechainFrame);

            const auto mainLufs = mainLoudness.getLufs();
            const auto sidechainLufs = sidechainLoudness.getLufs();

            // Substage C: Target LUFS retention buffer
            // Prevent immediate volume snap-back scaling by holding onto vocal anchor while they take a quick breath
            if (voicePresence > 0.7f)
            {
                heldVocalLevel = math::smoothTowards (heldVocalLevel, sidechainLufs, heldTrackingCoefficient);

                if (settings.anchorAutoMode)
                    autoAnchorPoint = math::smoothTowards (autoAnchorPoint, sidechainLufs, autoAnchorCoefficient);
            }
            else
            {
                heldVocalLevel = math::smoothTowards (heldVocalLevel, settings.noiseGateDb, heldReleaseCoefficient);
            }

            const auto effectiveAnchor = settings.anchorAutoMode ? autoAnchorPoint : settings.anchorManualDb;

            // ═══════════════════════════════════════════
            // Stage 2: Target Ratio Compensation Gain Math
            // ═══════════════════════════════════════════

            const auto activeGainDb = sidechainAvailable
                ? math::computeTargetCurveGainDb (heldVocalLevel, mainLufs, settings.balanceDb,
                                                  settings.dynamicsPercent, effectiveAnchor)
                : 0.0f;

            const auto idleGainDb = 0.0f; // Reset default resting bypass state

            // ═══════════════════════════════════════════
            // Stage 3: Crossfade logic mapping
            // ═══════════════════════════════════════════

            const auto rawTargetBlend = voicePresence * activeGainDb + (1.0f - voicePresence) * idleGainDb;
            
            // In Expansion mode, ducking rules become boosting rules causing upward expansions.
            const auto rawTargetGain = settings.expansionMode ? -rawTargetBlend : rawTargetBlend;

            // ═══════════════════════════════════════════
            // Stage 4: Clamp and Protect ranges
            // ═══════════════════════════════════════════

            targetGainDb = math::clampGainDb (rawTargetGain, settings.duckFloorDb, settings.boostCeilingDb);
            targetGainDb = juce::jlimit (-settings.rangeDb, settings.rangeDb, targetGainDb);

            // ═══════════════════════════════════════════
            // Stage 5: Non-linear response smoothing
            // ═══════════════════════════════════════════

            const auto smoothing = targetGainDb < smoothedGainDb ? attackCoefficient : releaseCoefficient;
            smoothedGainDb = math::smoothTowards (smoothedGainDb, targetGainDb, smoothing);

            // ═══════════════════════════════════════════
            // Stage 5.5: Spectral Duck EQ (Profile-driven)
            // ═══════════════════════════════════════════

            if (spectralDuckActive)
                spectralDuckEQ.updatePresence (voicePresence);

            // ═══════════════════════════════════════════
            // Stage 6: Render Multiplied Audio with Latency Match
            // ═══════════════════════════════════════════

            const auto gain = juce::Decibels::decibelsToGain<SampleType> (smoothedGainDb);

            for (int channel = 0; channel < mainChannelCount; ++channel)
            {
                // De-reference history circular buff samples to naturally push peaks backward in time
                auto delayedSample = lookAheadDelay.processSample (channel, mainFrame[static_cast<size_t> (channel)]);

                // Stage 5.5: Apply spectral notch filtering if a Profile is active
                if (spectralDuckActive)
                    delayedSample = spectralDuckEQ.processSample (channel, delayedSample);

                // IN-PLACE array update sent directly back to Host/DAW Memory Bus
                mainBuffer.getWritePointer (channel)[sample] = delayedSample * gain;
            }

            lookAheadDelay.advance();

            // ═══════════════════════════════════════════
            // State Dump to GPU/UI Message Thread
            // ═══════════════════════════════════════════

            if (--visualUpdateCounter <= 0)
            {
                visualUpdateCounter = visualUpdateInterval;

                // Push telemetry to lock-free atomic buffers for 90hz GUI render consumption.
                visualState.pushFrame (
                    sidechainLufs,
                    mainLufs,
                    smoothedGainDb,
                    voicePresence,
                    heldVocalLevel,
                    effectiveAnchor,
                    sidechainAvailable);
            }
        }
    }

    [[nodiscard]] int getLatencySamples() const noexcept
    {
        return latencySamples;
    }

    // ── Profile-driven Spectral Ducking ──

    /**
     * @brief Inject a Profile energy zone's parameters into the engine.
     * Updates both the macro RuntimeSettings and the SpectralDuckEQ bands.
     */
    void applyProfileZone (const EnergyZone& zone) noexcept
    {
        // Override macro settings from the profile zone
        RuntimeSettings profileSettings = settings;
        profileSettings.balanceDb = zone.balanceDb;
        profileSettings.dynamicsPercent = zone.dynamicsPercent;
        profileSettings.speedMs = zone.speedMs;
        profileSettings.rangeDb = zone.rangeDb;
        profileSettings.anchorManualDb = zone.anchorDb;
        profileSettings.anchorAutoMode = false; // Profile provides explicit anchor
        profileSettings.noiseGateDb = zone.noiseGateDb;
        profileSettings.duckFloorDb = zone.duckFloorDb;
        profileSettings.boostCeilingDb = zone.boostCeilingDb;
        profileSettings.presenceReleaseMs = zone.presenceReleaseMs;
        updateSettings (profileSettings);

        // Update spectral EQ bands
        spectralDuckEQ.updateBands (zone.duckBands);
    }

    void setSpectralDuckEnabled (bool enabled) noexcept
    {
        spectralDuckActive = enabled;

        if (! enabled)
            spectralDuckEQ.reset();
    }

    [[nodiscard]] bool isSpectralDuckEnabled() const noexcept { return spectralDuckActive; }

    [[nodiscard]] const SpectralDuckEQ<SampleType>& getSpectralDuckEQ() const noexcept { return spectralDuckEQ; }

private:
    double sampleRate = 44100.0;
    int mainChannelCount = 2;
    int sidechainChannelCount = 2;
    int latencySamples = 0;
    int visualUpdateInterval = 512;
    int visualUpdateCounter = 512;

    float attackCoefficient = 0.0f;
    float releaseCoefficient = 0.0f;
    float presenceAttackCoefficient = 0.0f;
    float presenceReleaseCoefficient = 0.0f;
    float heldTrackingCoefficient = 0.0f;
    float heldReleaseCoefficient = 0.0f;
    float autoAnchorCoefficient = 0.0f;

    float targetGainDb = 0.0f;
    float smoothedGainDb = 0.0f;
    float presenceEnvelope = 0.0f;
    float heldVocalLevel = -60.0f;
    float autoAnchorPoint = -24.0f;

    RuntimeSettings settings;
    LookAheadDelay<SampleType> lookAheadDelay;
    LoudnessEstimator<SampleType> mainLoudness;
    LoudnessEstimator<SampleType> sidechainLoudness;

    // Profile-driven spectral ducking
    SpectralDuckEQ<SampleType> spectralDuckEQ;
    bool spectralDuckActive = false;
};
} // namespace liveflow
