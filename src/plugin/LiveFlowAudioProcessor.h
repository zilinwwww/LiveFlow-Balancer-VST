#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "core/BalancerEngine.h"
#include "core/PluginParameters.h"
#include "core/VisualizationState.h"

namespace liveflow
{
class LiveFlowAudioProcessor final : public juce::AudioProcessor
{
public:
    LiveFlowAudioProcessor();
    ~LiveFlowAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    [[nodiscard]] juce::AudioProcessorEditor* createEditor() override;
    [[nodiscard]] bool hasEditor() const override { return true; }

    [[nodiscard]] const juce::String getName() const override { return JucePlugin_Name; }
    [[nodiscard]] bool acceptsMidi() const override { return false; }
    [[nodiscard]] bool producesMidi() const override { return false; }
    [[nodiscard]] bool isMidiEffect() const override { return false; }
    [[nodiscard]] double getTailLengthSeconds() const override { return 0.0; }
    [[nodiscard]] bool supportsDoublePrecisionProcessing() const override { return false; }

    [[nodiscard]] int getNumPrograms() override { return 1; }
    [[nodiscard]] int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    [[nodiscard]] const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    [[nodiscard]] juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return parameters; }
    [[nodiscard]] const juce::AudioProcessorValueTreeState& getValueTreeState() const noexcept { return parameters; }

    [[nodiscard]] VisualizationState& getVisualizationState() noexcept { return visualizationState; }
    [[nodiscard]] const VisualizationState& getVisualizationState() const noexcept { return visualizationState; }

private:
    template <typename SampleType>
    void processBlockInternal (juce::AudioBuffer<SampleType>& buffer,
                               BalancerEngine<SampleType>& engine,
                               juce::AudioBuffer<SampleType>& sidechainScratch);

    void updateLatencyIfNeeded (const int desiredLatencySamples);

    juce::AudioProcessorValueTreeState parameters;
    VisualizationState visualizationState;
    BalancerEngine<float> floatEngine;
    BalancerEngine<double> doubleEngine;
    juce::AudioBuffer<float> sidechainScratchFloat;
    juce::AudioBuffer<double> sidechainScratchDouble;
    int cachedLatencySamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiveFlowAudioProcessor)
};
} // namespace liveflow
