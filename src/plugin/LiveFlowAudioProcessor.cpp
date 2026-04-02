#include "LiveFlowAudioProcessor.h"

#include "LiveFlowAudioProcessorEditor.h"
#include "VST3HostDiagnostics.h"

namespace liveflow
{
namespace
{
template <typename SampleType>
bool busesShareStorage (const juce::AudioBuffer<SampleType>& left, const juce::AudioBuffer<SampleType>& right) noexcept
{
    if (left.getNumChannels() != right.getNumChannels())
        return false;

    if (left.getNumChannels() == 0)
        return true;

    return left.getReadPointer (0) == right.getReadPointer (0);
}

/**
 * @brief Zero-alloc routing function mapping standard discrete host sidechain buses into our processing memory block.
 * Supports flexible routing behaviors adopted by various DAWs (e.g., Logic Pro vs Ableton Live vs FL Studio).
 */
template <typename SampleType>
void copySidechainBusToScratch (juce::AudioProcessor& processor,
                                const juce::AudioBuffer<SampleType>& sourceBuffer,
                                juce::AudioBuffer<SampleType>& scratchBuffer)
{
    scratchBuffer.clear();

    if (processor.getBusCount (true) <= 1 || processor.getBus (true, 1) == nullptr || ! processor.getBus (true, 1)->isEnabled())
        return;

    const auto sidechainChannels = juce::jmin (scratchBuffer.getNumChannels(),
                                               processor.getChannelCountOfBus (true, 1));

    for (int channel = 0; channel < sidechainChannels; ++channel)
    {
        const auto absoluteChannel = processor.getChannelIndexInProcessBlockBuffer (true, 1, channel);

        if (absoluteChannel >= 0 && absoluteChannel < sourceBuffer.getNumChannels())
            juce::FloatVectorOperations::copy (scratchBuffer.getWritePointer (channel),
                                               sourceBuffer.getReadPointer (absoluteChannel),
                                               sourceBuffer.getNumSamples());
    }
}
} // namespace

LiveFlowAudioProcessor::LiveFlowAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withInput ("Sidechain", juce::AudioChannelSet::mono(), false)),
      parameters (*this, nullptr, "LiveFlowParameters", createParameterLayout())
{
}

void LiveFlowAudioProcessor::prepareToPlay (const double sampleRate, const int samplesPerBlock)
{
    const auto mainOutputChannels = juce::jmax (1, getChannelCountOfBus (false, 0));
    const auto sidechainChannels = juce::jmax (1, getChannelCountOfBus (true, 1));

    floatEngine.prepare (sampleRate, samplesPerBlock, mainOutputChannels, sidechainChannels);
    doubleEngine.prepare (sampleRate, samplesPerBlock, mainOutputChannels, sidechainChannels);
    sidechainScratchFloat.setSize (sidechainChannels, samplesPerBlock, false, true, true);
    sidechainScratchDouble.setSize (sidechainChannels, samplesPerBlock, false, true, true);
    visualizationState.reset();

    const auto settings = loadRuntimeSettings (parameters);
    floatEngine.updateSettings (settings);
    doubleEngine.updateSettings (settings);
    updateLatencyIfNeeded (0);
}

void LiveFlowAudioProcessor::releaseResources()
{
    floatEngine.reset();
    doubleEngine.reset();
    sidechainScratchFloat.clear();
    sidechainScratchDouble.clear();
}

bool LiveFlowAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainInput = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput != juce::AudioChannelSet::stereo() || mainOutput != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sidechain = layouts.getChannelSet (true, 1);
        const auto sidechainSupported = sidechain.isDisabled()
                                     || sidechain == juce::AudioChannelSet::mono();

        if (! sidechainSupported)
            return false;
    }

    return true;
}

template <typename SampleType>
void LiveFlowAudioProcessor::processBlockInternal (juce::AudioBuffer<SampleType>& buffer,
                                                   BalancerEngine<SampleType>& engine,
                                                   juce::AudioBuffer<SampleType>& sidechainScratch)
{
    // Scoped blocker preventing the CPU from catching on "Denormals" (Floating points infinitely near zero 
    // which normally throw older processors into severe pipeline stall panics taking 10x-100x longer to compute)
    juce::ScopedNoDenormals noDenormals;

    const auto settings = loadRuntimeSettings (parameters);
    engine.updateSettings (settings);
    updateLatencyIfNeeded (0);

    auto mainInputBus = getBusBuffer (buffer, true, 0);
    auto mainOutputBus = getBusBuffer (buffer, false, 0);

    if (! busesShareStorage (mainInputBus, mainOutputBus))
        mainOutputBus.makeCopyOf (mainInputBus, true);

    copySidechainBusToScratch (*this, buffer, sidechainScratch);
    const bool sidechainActive = getBusCount (true) > 1 && getBus (true, 1) != nullptr && getBus (true, 1)->isEnabled();
    const juce::AudioBuffer<SampleType>* sidechainBuffer = sidechainActive ? &sidechainScratch : nullptr;

    engine.process (mainOutputBus, sidechainBuffer, visualizationState);

    VisualizationState::Snapshot::Diagnostics diagnostics;
    diagnostics.rawChannelDb.fill (-100.0f);
    diagnostics.mappedSidechainDb.fill (-100.0f);
    diagnostics.processBufferChannels = buffer.getNumChannels();
    diagnostics.processBlockSamples = buffer.getNumSamples();
    diagnostics.totalInputChannels = getTotalNumInputChannels();
    diagnostics.totalOutputChannels = getTotalNumOutputChannels();
    diagnostics.inputBusCount = getBusCount (true);
    diagnostics.outputBusCount = getBusCount (false);
    const auto wrapperSnapshot = liveflow::vst3debug::capture();
    diagnostics.wrapperNumInputs = wrapperSnapshot.numInputs;
    diagnostics.wrapperNumOutputs = wrapperSnapshot.numOutputs;
    diagnostics.wrapperNumSamples = wrapperSnapshot.numSamples;
    diagnostics.mainInputChannels = getChannelCountOfBus (true, 0);
    diagnostics.mainOutputChannels = getChannelCountOfBus (false, 0);
    diagnostics.sidechainBusPresent = getBusCount (true) > 1 && getBus (true, 1) != nullptr;
    diagnostics.sidechainBusEnabled = diagnostics.sidechainBusPresent && getBus (true, 1)->isEnabled();
    diagnostics.sidechainChannels = diagnostics.sidechainBusPresent ? getChannelCountOfBus (true, 1) : 0;
    diagnostics.sidechainMap0 = diagnostics.sidechainChannels > 0 ? getChannelIndexInProcessBlockBuffer (true, 1, 0) : -1;
    diagnostics.sidechainMap1 = diagnostics.sidechainChannels > 1 ? getChannelIndexInProcessBlockBuffer (true, 1, 1) : -1;
    diagnostics.wrapperInputBusChannels = wrapperSnapshot.inputBusChannels;
    diagnostics.wrapperInputBusDb = wrapperSnapshot.inputBusDb;
    diagnostics.wrapperInputBusHasBuffers = wrapperSnapshot.inputBusHasBuffers;
    diagnostics.wrapperInputBusSilentAll = wrapperSnapshot.inputBusSilentAll;

    for (int channel = 0; channel < juce::jmin (buffer.getNumChannels(), VisualizationState::diagnosticChannels); ++channel)
    {
        auto peak = static_cast<SampleType> (0);
        const auto* read = buffer.getReadPointer (channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            peak = juce::jmax (peak, std::abs (read[sample]));

        diagnostics.rawChannelDb[static_cast<size_t> (channel)] = juce::Decibels::gainToDecibels (static_cast<float> (peak), -100.0f);
    }

    for (int channel = 0; channel < juce::jmin (sidechainScratch.getNumChannels(), 2); ++channel)
    {
        auto peak = static_cast<SampleType> (0);
        const auto* read = sidechainScratch.getReadPointer (channel);

        for (int sample = 0; sample < sidechainScratch.getNumSamples(); ++sample)
            peak = juce::jmax (peak, std::abs (read[sample]));

        diagnostics.mappedSidechainDb[static_cast<size_t> (channel)] = juce::Decibels::gainToDecibels (static_cast<float> (peak), -100.0f);
    }

    visualizationState.updateDiagnostics (diagnostics);
}

void LiveFlowAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    processBlockInternal (buffer, floatEngine, sidechainScratchFloat);
}

void LiveFlowAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    processBlockInternal (buffer, doubleEngine, sidechainScratchDouble);
}

juce::AudioProcessorEditor* LiveFlowAudioProcessor::createEditor()
{
    return new LiveFlowAudioProcessorEditor (*this);
}

void LiveFlowAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void LiveFlowAudioProcessor::setStateInformation (const void* data, const int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

void LiveFlowAudioProcessor::updateLatencyIfNeeded (const int desiredLatencySamples)
{
    if (desiredLatencySamples != cachedLatencySamples)
    {
        cachedLatencySamples = desiredLatencySamples;
        setLatencySamples (cachedLatencySamples);
    }
}
} // namespace liveflow

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new liveflow::LiveFlowAudioProcessor();
}
