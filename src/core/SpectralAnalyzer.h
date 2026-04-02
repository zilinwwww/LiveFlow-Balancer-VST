#pragma once

#include <array>
#include <cmath>
#include <vector>

#include <juce_dsp/juce_dsp.h>

#include "TrackProfile.h"

namespace liveflow
{
/**
 * @class SpectralAnalyzer
 * @brief Offline FFT-based spectral analysis engine for the Smart Track Profiler.
 *
 * This class runs EXCLUSIVELY on a background thread (never the audio thread).
 * It processes recorded audio buffers to extract:
 *   1. Per-frame spectral magnitude for both backing and vocal tracks
 *   2. Spectral collision map (where both tracks have high energy simultaneously)
 *   3. Recommended duck band parameters (center Hz, Q, depth)
 *
 * The analysis is partitioned by energy zones: frames are grouped by their
 * backing LUFS level, and collision analysis is performed per-zone independently.
 */
class SpectralAnalyzer
{
public:
    static constexpr int fftOrder = 10;                    // 2^10 = 1024 point FFT
    static constexpr int fftSize = 1 << fftOrder;         // 1024 samples
    static constexpr int fftBins = fftSize / 2 + 1;       // 513 frequency bins
    static constexpr int hopSize = fftSize / 2;            // 50% overlap

    /**
     * @brief Analyze recorded audio and generate a complete TrackProfile.
     *
     * @param backingBuffer  The recorded backing track audio (interleaved or separate channels)
     * @param vocalBuffer    The recorded sidechain vocal audio
     * @param sampleRate     The sample rate of the recorded audio
     * @param numZones       Number of energy zones to create (2-8)
     * @param numBands       Number of duck EQ bands per zone (1-8)
     * @param profileName    Name for the generated profile
     * @return A fully populated TrackProfile
     */
    static TrackProfile analyze (const juce::AudioBuffer<float>& backingBuffer,
                                 const juce::AudioBuffer<float>& vocalBuffer,
                                 double sampleRate,
                                 int numZones,
                                 int numBands,
                                 const juce::String& profileName)
    {
        TrackProfile profile;
        profile.name = profileName;
        profile.created = juce::Time::getCurrentTime();
        profile.sampleRate = sampleRate;

        const auto numSamples = juce::jmin (backingBuffer.getNumSamples(), vocalBuffer.getNumSamples());

        if (numSamples < fftSize)
        {
            // Not enough audio — return a default single-zone profile
            profile.zones.push_back (EnergyZone {});
            return profile;
        }

        // ── Step 1: Compute per-frame LUFS for the backing track ──
        auto frameLufs = computeFrameLufs (backingBuffer, sampleRate, numSamples);

        // ── Step 2: Determine zone boundaries from LUFS percentiles ──
        auto sortedLufs = frameLufs;
        std::sort (sortedLufs.begin(), sortedLufs.end());

        std::vector<float> zoneBoundaries;
        zoneBoundaries.push_back (-100.0f); // Lower bound of first zone

        for (int z = 1; z < numZones; ++z)
        {
            const auto percentile = static_cast<float> (z) / static_cast<float> (numZones);
            const auto index = static_cast<size_t> (percentile * static_cast<float> (sortedLufs.size() - 1));
            zoneBoundaries.push_back (sortedLufs[index]);
        }

        zoneBoundaries.push_back (0.0f); // Upper bound of last zone

        // ── Step 3: FFT analysis — compute spectral collision per zone ──
        juce::dsp::FFT fft (fftOrder);
        juce::dsp::WindowingFunction<float> window (static_cast<size_t> (fftSize),
                                                     juce::dsp::WindowingFunction<float>::hann);

        // Accumulator: per-zone, per-bin average magnitude for both tracks
        std::vector<std::vector<float>> zoneBackingMagnitude (static_cast<size_t> (numZones),
                                                               std::vector<float> (fftBins, 0.0f));
        std::vector<std::vector<float>> zoneVocalMagnitude (static_cast<size_t> (numZones),
                                                             std::vector<float> (fftBins, 0.0f));
        std::vector<int> zoneFrameCount (static_cast<size_t> (numZones), 0);

        // Per-zone LUFS statistics
        std::vector<float> zoneVocalLufsSum (static_cast<size_t> (numZones), 0.0f);

        std::vector<float> fftBuffer (static_cast<size_t> (fftSize * 2), 0.0f);

        int frameIndex = 0;

        for (int pos = 0; pos + fftSize <= numSamples; pos += hopSize, ++frameIndex)
        {
            // Determine which zone this frame belongs to
            const auto frameLufsValue = frameIndex < static_cast<int> (frameLufs.size())
                                            ? frameLufs[static_cast<size_t> (frameIndex)]
                                            : -100.0f;

            int zoneIndex = numZones - 1;

            for (int z = numZones - 1; z >= 1; --z)
            {
                if (frameLufsValue < zoneBoundaries[static_cast<size_t> (z)])
                {
                    zoneIndex = z - 1;
                }
                else
                {
                    break;
                }
            }

            zoneIndex = juce::jlimit (0, numZones - 1, zoneIndex);
            auto zIdx = static_cast<size_t> (zoneIndex);
            zoneFrameCount[zIdx]++;

            // Accumulate vocal LUFS per zone for anchor calculation
            const auto vocalFrameLufs = computeSingleFrameLufs (vocalBuffer, pos, fftSize, sampleRate);
            zoneVocalLufsSum[zIdx] += vocalFrameLufs;

            // ── Backing track FFT ──
            extractAndTransform (backingBuffer, pos, fft, window, fftBuffer);

            for (int bin = 0; bin < fftBins; ++bin)
            {
                auto bIdx = static_cast<size_t> (bin);
                const auto re = fftBuffer[bIdx * 2];
                const auto im = fftBuffer[bIdx * 2 + 1];
                zoneBackingMagnitude[zIdx][bIdx] += std::sqrt (re * re + im * im);
            }

            // ── Vocal track FFT ──
            extractAndTransform (vocalBuffer, pos, fft, window, fftBuffer);

            for (int bin = 0; bin < fftBins; ++bin)
            {
                auto bIdx = static_cast<size_t> (bin);
                const auto re = fftBuffer[bIdx * 2];
                const auto im = fftBuffer[bIdx * 2 + 1];
                zoneVocalMagnitude[zIdx][bIdx] += std::sqrt (re * re + im * im);
            }
        }

        // ── Step 4: Normalize and find collision peaks per zone ──
        for (int z = 0; z < numZones; ++z)
        {
            auto zIdx = static_cast<size_t> (z);
            EnergyZone zone;
            zone.lufsLower = zoneBoundaries[zIdx];
            zone.lufsUpper = zoneBoundaries[zIdx + 1];

            const auto count = juce::jmax (1, zoneFrameCount[zIdx]);

            // Normalize accumulated magnitudes
            for (int bin = 0; bin < fftBins; ++bin)
            {
                auto bIdx = static_cast<size_t> (bin);
                zoneBackingMagnitude[zIdx][bIdx] /= static_cast<float> (count);
                zoneVocalMagnitude[zIdx][bIdx] /= static_cast<float> (count);
            }

            // Compute collision score: product of backing × vocal magnitude
            std::vector<float> collisionScore (fftBins, 0.0f);

            for (int bin = 0; bin < fftBins; ++bin)
            {
                auto bIdx = static_cast<size_t> (bin);
                collisionScore[bIdx] = zoneBackingMagnitude[zIdx][bIdx]
                                     * zoneVocalMagnitude[zIdx][bIdx];
            }

            // Find top N collision peaks
            zone.duckBands = findTopCollisionBands (collisionScore, sampleRate, numBands);

            // Scale duck depth by zone intensity
            const auto zoneIntensity = static_cast<float> (z + 1) / static_cast<float> (numZones);

            for (auto& band : zone.duckBands)
                band.depthDb *= (0.5f + 0.5f * zoneIntensity); // Quiet zones get half depth

            // Compute macro parameters from statistics
            const auto avgVocalLufs = zoneVocalLufsSum[zIdx] / static_cast<float> (count);
            zone.anchorDb = juce::jlimit (-60.0f, -5.0f, avgVocalLufs);
            zone.balanceDb = 3.0f + zoneIntensity * 2.0f; // Louder zones need more separation
            zone.dynamicsPercent = zoneIntensity * 40.0f;  // More aggressive dynamics in loud zones
            zone.speedMs = 200.0f - zoneIntensity * 100.0f; // Faster response in loud zones
            zone.rangeDb = 4.0f + zoneIntensity * 6.0f;
            zone.noiseGateDb = -45.0f;
            zone.duckFloorDb = 8.0f + zoneIntensity * 8.0f;
            zone.boostCeilingDb = 6.0f;
            zone.presenceReleaseMs = 120.0f - zoneIntensity * 40.0f;

            profile.zones.push_back (zone);
        }

        return profile;
    }

private:
    /**
     * @brief Compute approximate LUFS for each hop-sized frame of the backing track.
     */
    static std::vector<float> computeFrameLufs (const juce::AudioBuffer<float>& buffer,
                                                 double /*sampleRate*/,
                                                 int numSamples)
    {
        std::vector<float> result;
        result.reserve (static_cast<size_t> (numSamples / hopSize));

        for (int pos = 0; pos + fftSize <= numSamples; pos += hopSize)
        {
            result.push_back (computeSingleFrameLufs (buffer, pos, fftSize, 0.0));
        }

        return result;
    }

    /**
     * @brief Compute RMS-based approximate LUFS for a single frame.
     */
    static float computeSingleFrameLufs (const juce::AudioBuffer<float>& buffer,
                                          int startSample,
                                          int length,
                                          double /*sampleRate*/)
    {
        float energy = 0.0f;
        const auto channels = buffer.getNumChannels();

        for (int ch = 0; ch < channels; ++ch)
        {
            const auto* data = buffer.getReadPointer (ch);

            for (int i = startSample; i < startSample + length && i < buffer.getNumSamples(); ++i)
            {
                energy += data[i] * data[i];
            }
        }

        energy /= static_cast<float> (length * juce::jmax (1, channels));

        return -0.691f + 10.0f * std::log10 (juce::jmax (1.0e-12f, energy));
    }

    /**
     * @brief Extract a windowed frame from an audio buffer and perform FFT in-place.
     */
    static void extractAndTransform (const juce::AudioBuffer<float>& buffer,
                                      int startSample,
                                      juce::dsp::FFT& fft,
                                      juce::dsp::WindowingFunction<float>& window,
                                      std::vector<float>& fftBuffer)
    {
        std::fill (fftBuffer.begin(), fftBuffer.end(), 0.0f);

        // Mix down to mono for spectral analysis
        const auto channels = buffer.getNumChannels();

        for (int ch = 0; ch < channels; ++ch)
        {
            const auto* data = buffer.getReadPointer (ch);

            for (int i = 0; i < fftSize && (startSample + i) < buffer.getNumSamples(); ++i)
                fftBuffer[static_cast<size_t> (i * 2)] += data[startSample + i];
        }

        if (channels > 1)
        {
            for (int i = 0; i < fftSize; ++i)
                fftBuffer[static_cast<size_t> (i * 2)] /= static_cast<float> (channels);
        }

        // Extract real part for windowing
        std::vector<float> realPart (static_cast<size_t> (fftSize));

        for (int i = 0; i < fftSize; ++i)
            realPart[static_cast<size_t> (i)] = fftBuffer[static_cast<size_t> (i * 2)];

        window.multiplyWithWindowingTable (realPart.data(), static_cast<size_t> (fftSize));

        // Pack back into interleaved complex format
        for (int i = 0; i < fftSize; ++i)
        {
            fftBuffer[static_cast<size_t> (i * 2)] = realPart[static_cast<size_t> (i)];
            fftBuffer[static_cast<size_t> (i * 2 + 1)] = 0.0f;
        }

        fft.performFrequencyOnlyForwardTransform (fftBuffer.data(), true);
    }

    /**
     * @brief Find the top N frequency bins with the highest collision score.
     *
     * Returns DuckBand parameters for each peak, with appropriate Q and depth values.
     * Merges nearby peaks (within 1 octave) to avoid redundant overlapping notches.
     */
    static std::vector<DuckBand> findTopCollisionBands (const std::vector<float>& collisionScore,
                                                         double sampleRate,
                                                         int numBands)
    {
        struct Peak
        {
            int bin;
            float score;
        };

        // Find all local maxima
        std::vector<Peak> peaks;

        for (int bin = 2; bin < static_cast<int> (collisionScore.size()) - 2; ++bin)
        {
            auto bIdx = static_cast<size_t> (bin);

            if (collisionScore[bIdx] > collisionScore[bIdx - 1]
                && collisionScore[bIdx] > collisionScore[bIdx + 1]
                && collisionScore[bIdx] > 1.0e-8f)
            {
                peaks.push_back ({ bin, collisionScore[bIdx] });
            }
        }

        // Sort by score descending
        std::sort (peaks.begin(), peaks.end(),
                   [] (const Peak& a, const Peak& b) { return a.score > b.score; });

        // Select top N peaks, enforcing minimum 1-octave spacing
        std::vector<DuckBand> result;
        const auto binToHz = [&] (int bin) { return static_cast<float> (bin) * static_cast<float> (sampleRate) / static_cast<float> (fftSize); };

        for (const auto& peak : peaks)
        {
            if (static_cast<int> (result.size()) >= numBands)
                break;

            const auto hz = binToHz (peak.bin);

            if (hz < 80.0f || hz > 12000.0f)
                continue; // Skip extreme frequencies

            // Check minimum spacing
            bool tooClose = false;

            for (const auto& existing : result)
            {
                const auto ratio = hz / existing.centerHz;

                if (ratio > 0.5f && ratio < 2.0f) // Within one octave
                {
                    tooClose = true;
                    break;
                }
            }

            if (tooClose)
                continue;

            DuckBand band;
            band.centerHz = hz;
            band.bandwidthQ = 2.0f; // Moderate Q for natural-sounding notches
            band.depthDb = juce::jlimit (2.0f, 12.0f, peak.score * 500.0f); // Scale score to dB
            band.enabled = true;
            result.push_back (band);
        }

        // If we didn't find enough peaks, fill with sensible defaults
        while (static_cast<int> (result.size()) < numBands)
        {
            // Default bands at typical vocal frequency ranges
            static const float defaultFreqs[] = { 1000.0f, 2500.0f, 500.0f, 4000.0f, 800.0f, 1500.0f, 3000.0f, 6000.0f };
            const auto idx = result.size();

            DuckBand band;
            band.centerHz = defaultFreqs[idx % 8];
            band.bandwidthQ = 1.5f;
            band.depthDb = 3.0f;
            band.enabled = true;
            result.push_back (band);
        }

        return result;
    }
};

} // namespace liveflow
