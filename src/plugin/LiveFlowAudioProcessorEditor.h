#pragma once

#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_core/juce_core.h>

namespace liveflow
{
class LiveFlowAudioProcessor;

class LiveFlowAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit LiveFlowAudioProcessorEditor (LiveFlowAudioProcessor&);
    ~LiveFlowAudioProcessorEditor() override;

    void resized() override;
    void timerCallback() override;

private:
    juce::WebBrowserComponent::Options createWebOptions();
    void sendInitialStateParams();

    LiveFlowAudioProcessor& processor;
    juce::WebBrowserComponent webBrowser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiveFlowAudioProcessorEditor)
};
} // namespace liveflow
