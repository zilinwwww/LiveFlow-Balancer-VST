#pragma once

#include <array>
#include <cmath>

#include <juce_dsp/juce_dsp.h>

namespace liveflow
{
/**
 * @class LoudnessEstimator
 * @brief Computes perceived loudness (LUFS) per-frame using an adapted ITU-R BS.1770 K-Weighting filter.
 * 
 * To accurately model the non-linear human ear frequency response, this class operates a sequential 
 * dual-filter stage: A gentle high-shelf boost for intelligibility, and a sharp high-pass filter 
 * at 38Hz to ruthlessly discard DC offsets and inaudible sub-bass impact from the energy calculation.
 *
 * @tparam SampleType Underlying float format (float/double)
 */
template <typename SampleType>
class LoudnessEstimator
{
public:
    static constexpr int maxChannels = 2;

    void prepare (const double newSampleRate, const int numChannels)
    {
        sampleRate = newSampleRate;
        channelCount = juce::jlimit (1, maxChannels, numChannels);

        auto highPassCoefficients = juce::dsp::IIR::Coefficients<SampleType>::makeHighPass (sampleRate,
                                                                                             static_cast<SampleType> (38.0));
        auto highShelfCoefficients = juce::dsp::IIR::Coefficients<SampleType>::makeHighShelf (sampleRate,
                                                                                                static_cast<SampleType> (1681.974450955533),
                                                                                                static_cast<SampleType> (0.70710678118),
                                                                                                juce::Decibels::decibelsToGain<SampleType> (4.0f));

        for (int channel = 0; channel < channelCount; ++channel)
        {
            highPassFilters[static_cast<size_t> (channel)].coefficients = highPassCoefficients;
            highShelfFilters[static_cast<size_t> (channel)].coefficients = highShelfCoefficients;
            highPassFilters[static_cast<size_t> (channel)].reset();
            highShelfFilters[static_cast<size_t> (channel)].reset();
        }

        energyCoefficient = std::exp (-1.0 / (0.4 * sampleRate));
        reset();
    }

    void reset() noexcept
    {
        for (int channel = 0; channel < channelCount; ++channel)
        {
            highPassFilters[static_cast<size_t> (channel)].reset();
            highShelfFilters[static_cast<size_t> (channel)].reset();
        }

        weightedEnergy = static_cast<SampleType> (1.0e-8);
    }

    void pushFrame (const std::array<SampleType, maxChannels>& frame) noexcept
    {
        auto frameEnergy = static_cast<SampleType> (0);

        for (int channel = 0; channel < channelCount; ++channel)
        {
            auto filtered = highShelfFilters[static_cast<size_t> (channel)].processSample (frame[static_cast<size_t> (channel)]);
            filtered = highPassFilters[static_cast<size_t> (channel)].processSample (filtered);
            frameEnergy += filtered * filtered;
        }

        frameEnergy /= static_cast<SampleType> (juce::jmax (1, channelCount));
        weightedEnergy = static_cast<SampleType> (energyCoefficient) * weightedEnergy
                       + static_cast<SampleType> (1.0 - energyCoefficient) * frameEnergy;
    }

    [[nodiscard]] float getLufs() const noexcept
    {
        return static_cast<float> (-0.691 + 10.0 * std::log10 (juce::jmax (static_cast<SampleType> (1.0e-12), weightedEnergy)));
    }

private:
    std::array<juce::dsp::IIR::Filter<SampleType>, maxChannels> highPassFilters;
    std::array<juce::dsp::IIR::Filter<SampleType>, maxChannels> highShelfFilters;
    double sampleRate = 44100.0;
    double energyCoefficient = 0.0;
    int channelCount = maxChannels;
    SampleType weightedEnergy = static_cast<SampleType> (1.0e-8);
};
} // namespace liveflow
