#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace liveflow
{
/**
 * @class LookAheadDelay
 * @brief Zero-allocation circular buffer designed to precisely delay tracking audio.
 * 
 * Used to shift the accompaniment audio backwards in time relative to the sidechain signal.
 * This guarantees the DSP envelope trackers can react "before" the transient actually hits the speakers,
 * preventing fast volume spikes (like kick drums or consonants) from escaping the ducking curve un-attenuated.
 * 
 * Employs a strict wrap-around integer index to avoid costly bounds checking inside the hot-path process block.
 * 
 * @tparam SampleType The audio floating point type format (float or double)
 */
template <typename SampleType>
class LookAheadDelay
{
public:
    /**
     * @brief Allocates the circular ring buffer memory constraint.
     */
    void prepare (const int numChannels, const int maximumDelaySamples)
    {
        delayBuffer.setSize (numChannels, juce::jmax (1, maximumDelaySamples + 1), false, true, true);
        delayBuffer.clear();
        writeIndex = 0;
        delaySamples = 0;
    }

    /**
     * @brief Clears the delay lines, silencing any echoing remnants.
     */
    void reset() noexcept
    {
        delayBuffer.clear();
        writeIndex = 0;
    }

    /**
     * @brief Modifies the look-ahead latency at runtime. Strictly constraints memory boundaries.
     */
    void setDelaySamples (const int newDelaySamples) noexcept
    {
        delaySamples = juce::jlimit (0, juce::jmax (0, delayBuffer.getNumSamples() - 1), newDelaySamples);
    }

    [[nodiscard]] int getDelaySamples() const noexcept
    {
        return delaySamples;
    }

    /**
     * @brief Pushes a current sample into the future, and returns a sample from the past.
     * 
     * Applies modular arithmetic to find the trailing read offset.
     */
    SampleType processSample (const int channel, const SampleType inputSample) noexcept
    {
        auto* channelData = delayBuffer.getWritePointer (channel);
        channelData[writeIndex] = inputSample;

        auto readIndex = writeIndex - delaySamples;
        if (readIndex < 0)
            readIndex += delayBuffer.getNumSamples();

        return channelData[readIndex];
    }

    /**
     * @brief Increments the global timeline ptr, wrapping around the ring buffer seamlessly.
     */
    void advance() noexcept
    {
        ++writeIndex;
        if (writeIndex >= delayBuffer.getNumSamples())
            writeIndex = 0;
    }

private:
    juce::AudioBuffer<SampleType> delayBuffer;
    int writeIndex = 0;
    int delaySamples = 0;
};
} // namespace liveflow
