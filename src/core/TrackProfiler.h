#pragma once

#include <atomic>
#include <functional>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "SpectralAnalyzer.h"
#include "TrackProfile.h"

namespace liveflow
{
/**
 * @enum ProfilerState
 * @brief State machine states for the Track Profiler lifecycle.
 */
enum class ProfilerState
{
    Idle,       ///< Ready to start recording
    Recording,  ///< Actively capturing audio from both tracks
    Analyzing,  ///< Background thread running FFT analysis
    Done,       ///< Profile generated and available
    Error       ///< Analysis failed (e.g. not enough audio)
};

/**
 * @class TrackProfiler
 * @brief Manages the "Listen → Record → Analyze → Profile" lifecycle.
 *
 * Recording (audio thread):
 *   Copies incoming main + sidechain samples into pre-allocated ring buffers.
 *   Zero malloc — buffers are allocated in prepare().
 *
 * Analysis (background thread):
 *   When recording stops, launches a juce::Thread to run SpectralAnalyzer.
 *   The UI polls state/progress via atomics.
 *
 * Memory:
 *   10 min × 48kHz × 2ch × 4B = ~230 MB (worst case)
 *   Allocated once at prepare(), freed at destructor.
 */
class TrackProfiler : private juce::Thread
{
public:
    TrackProfiler() : juce::Thread ("ProfilerAnalysis") {}

    ~TrackProfiler() override
    {
        stopThread (5000);
    }

    /**
     * @brief Pre-allocate recording buffers.
     *
     * @param sampleRate    Host sample rate
     * @param maxMinutes    Maximum recording duration in minutes (1-10)
     * @param numChannels   Number of audio channels (typically 2)
     */
    void prepare (double newSampleRate, int maxMinutes, int numChannels)
    {
        sampleRate = newSampleRate;
        channelCount = juce::jlimit (1, 2, numChannels);

        const auto clampedMinutes = juce::jlimit (1, 10, maxMinutes);
        maxRecordingSamples = static_cast<int> (sampleRate * 60.0 * clampedMinutes);

        backingBuffer.setSize (channelCount, maxRecordingSamples, false, true, false);
        vocalBuffer.setSize (channelCount, maxRecordingSamples, false, true, false);

        reset();
    }

    /**
     * @brief Clear all state and buffers.
     */
    void reset()
    {
        stopThread (2000);

        backingBuffer.clear();
        vocalBuffer.clear();
        recordPosition = 0;
        state.store (static_cast<int> (ProfilerState::Idle));
        progress.store (0.0f);
        lastGeneratedProfile = TrackProfile {};
    }

    // ────────────────────────────────────────────
    // Recording Control (called from Message Thread)
    // ────────────────────────────────────────────

    /**
     * @brief Start recording. Call from the UI/Message thread.
     */
    void startRecording()
    {
        if (getState() != ProfilerState::Idle && getState() != ProfilerState::Done)
            return;

        backingBuffer.clear();
        vocalBuffer.clear();
        recordPosition = 0;
        progress.store (0.0f);
        state.store (static_cast<int> (ProfilerState::Recording));
    }

    /**
     * @brief Stop recording and kick off background analysis.
     *
     * @param numZones  Number of energy zones to generate (2-8)
     * @param numBands  Number of EQ bands per zone (1-8)
     * @param name      Profile name
     */
    void stopRecording (int numZones, int numBands, const juce::String& name)
    {
        if (getState() != ProfilerState::Recording)
            return;

        pendingNumZones = juce::jlimit (TrackProfile::minZones, TrackProfile::maxZones, numZones);
        pendingNumBands = juce::jlimit (TrackProfile::minBands, TrackProfile::maxBands, numBands);
        pendingName = name;

        state.store (static_cast<int> (ProfilerState::Analyzing));
        progress.store (0.0f);
        startThread (juce::Thread::Priority::background);
    }

    /**
     * @brief Cancel recording without analyzing.
     */
    void cancelRecording()
    {
        if (getState() == ProfilerState::Recording)
        {
            state.store (static_cast<int> (ProfilerState::Idle));
            progress.store (0.0f);
        }
        else if (getState() == ProfilerState::Analyzing)
        {
            signalThreadShouldExit();
            stopThread (3000);
            state.store (static_cast<int> (ProfilerState::Idle));
            progress.store (0.0f);
        }
    }

    // ────────────────────────────────────────────
    // Audio Thread Interface (called from processBlock)
    // ────────────────────────────────────────────

    /**
     * @brief Feed audio samples into the recorder. Called from the audio thread.
     *
     * This is a zero-allocation memcpy into pre-allocated buffers.
     * Automatically stops recording when the buffer is full.
     *
     * @param mainBlock      Current block of backing track audio
     * @param sidechainBlock Current block of sidechain vocal audio (may be nullptr)
     */
    void pushAudioBlock (const juce::AudioBuffer<float>& mainBlock,
                         const juce::AudioBuffer<float>* sidechainBlock) noexcept
    {
        if (getState() != ProfilerState::Recording)
            return;

        const auto numSamples = mainBlock.getNumSamples();
        const auto remaining = maxRecordingSamples - recordPosition;

        if (remaining <= 0)
        {
            // Buffer full — auto-transition (will be picked up by UI to call stopRecording)
            return;
        }

        const auto toCopy = juce::jmin (numSamples, remaining);

        // Copy backing track
        for (int ch = 0; ch < juce::jmin (channelCount, mainBlock.getNumChannels()); ++ch)
        {
            juce::FloatVectorOperations::copy (
                backingBuffer.getWritePointer (ch, recordPosition),
                mainBlock.getReadPointer (ch),
                toCopy);
        }

        // Copy vocal sidechain
        if (sidechainBlock != nullptr)
        {
            for (int ch = 0; ch < juce::jmin (channelCount, sidechainBlock->getNumChannels()); ++ch)
            {
                juce::FloatVectorOperations::copy (
                    vocalBuffer.getWritePointer (ch, recordPosition),
                    sidechainBlock->getReadPointer (ch),
                    toCopy);
            }
        }

        recordPosition += toCopy;

        // Update progress atomically
        progress.store (static_cast<float> (recordPosition) / static_cast<float> (maxRecordingSamples));
    }

    /**
     * @brief Check if the recording buffer is full.
     */
    [[nodiscard]] bool isBufferFull() const noexcept
    {
        return recordPosition >= maxRecordingSamples;
    }

    // ────────────────────────────────────────────
    // State Query (thread-safe)
    // ────────────────────────────────────────────

    [[nodiscard]] ProfilerState getState() const noexcept
    {
        return static_cast<ProfilerState> (state.load (std::memory_order_relaxed));
    }

    [[nodiscard]] float getProgress() const noexcept
    {
        return progress.load (std::memory_order_relaxed);
    }

    [[nodiscard]] float getRecordedSeconds() const noexcept
    {
        return static_cast<float> (recordPosition) / static_cast<float> (juce::jmax (1.0, sampleRate));
    }

    [[nodiscard]] float getMaxSeconds() const noexcept
    {
        return static_cast<float> (maxRecordingSamples) / static_cast<float> (juce::jmax (1.0, sampleRate));
    }

    /**
     * @brief Retrieve the last generated profile. Only valid when state == Done.
     */
    [[nodiscard]] const TrackProfile& getGeneratedProfile() const noexcept
    {
        return lastGeneratedProfile;
    }

    /**
     * @brief Acknowledge the profile and return to Idle state.
     */
    void acknowledgeProfile()
    {
        if (getState() == ProfilerState::Done || getState() == ProfilerState::Error)
            state.store (static_cast<int> (ProfilerState::Idle));
    }

private:
    // ────────────────────────────────────────────
    // Background Analysis Thread
    // ────────────────────────────────────────────

    void run() override
    {
        progress.store (0.1f);

        if (recordPosition < SpectralAnalyzer::fftSize * 4)
        {
            // Not enough audio recorded
            state.store (static_cast<int> (ProfilerState::Error));
            return;
        }

        // Create trimmed copies of the recorded audio (only the actually-recorded portion)
        juce::AudioBuffer<float> trimmedBacking (channelCount, recordPosition);
        juce::AudioBuffer<float> trimmedVocal (channelCount, recordPosition);

        for (int ch = 0; ch < channelCount; ++ch)
        {
            juce::FloatVectorOperations::copy (
                trimmedBacking.getWritePointer (ch),
                backingBuffer.getReadPointer (ch),
                recordPosition);

            juce::FloatVectorOperations::copy (
                trimmedVocal.getWritePointer (ch),
                vocalBuffer.getReadPointer (ch),
                recordPosition);
        }

        progress.store (0.3f);

        if (threadShouldExit())
            return;

        // Run spectral analysis
        lastGeneratedProfile = SpectralAnalyzer::analyze (
            trimmedBacking,
            trimmedVocal,
            sampleRate,
            pendingNumZones,
            pendingNumBands,
            pendingName);

        progress.store (1.0f);
        state.store (static_cast<int> (ProfilerState::Done));
    }

    // ── State ──
    std::atomic<int> state { static_cast<int> (ProfilerState::Idle) };
    std::atomic<float> progress { 0.0f };

    // ── Recording buffers ──
    juce::AudioBuffer<float> backingBuffer;
    juce::AudioBuffer<float> vocalBuffer;
    int recordPosition = 0;
    int maxRecordingSamples = 0;
    int channelCount = 2;
    double sampleRate = 44100.0;

    // ── Analysis parameters (set before analysis thread starts) ──
    int pendingNumZones = TrackProfile::defaultZones;
    int pendingNumBands = TrackProfile::defaultBands;
    juce::String pendingName;

    // ── Result ──
    TrackProfile lastGeneratedProfile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackProfiler)
};

} // namespace liveflow
