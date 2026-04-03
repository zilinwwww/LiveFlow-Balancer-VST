#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "ui/CompactKnob.h"
#include "ui/GainTimelineComponent.h"
#include "ui/HelpOverlay.h"
#include "ui/LiveFlowLookAndFeel.h"
#include "ui/PresenceIndicator.h"
#include "ui/Translations.h"
#include "ui/VisualizerComponent.h"
#include "ui/ActivationOverlay.h"
#include "ui/AboutOverlay.h"

namespace liveflow
{
class LiveFlowAudioProcessor;

/**
 * LiveFlowAudioProcessorEditor — FabFilter-style layout:
 *
 *  ┌─────────────────────────────────────────────────────┐
 *  │  LiveFlow Balancer                 [badges]          │
 *  │  ┌───────────────────────────────────────────────┐  │
 *  │  │              Interactive Canvas               │  │
 *  │  │      (target curve, state point, trail)       │  │
 *  │  └───────────────────────────────────────────────┘  │
 *  │  ┌──────┐ ┌──────────────────────────────┐ ┌────┐  │
 *  │  │Voice │ │     Gain Timeline            │ │gain│  │
 *  │  │ 0.87 │ │ ████░░░░░░░░░░░░░░░░░░░░░░░ │ │+2dB│  │
 *  │  └──────┘ └──────────────────────────────┘ └────┘  │
 *  │  [Balance] [Dynamics] [Speed] [Range] [Anchor]      │
 *  │  ┌── Expert ───────────────────────────────────┐    │
 *  │  │ Boost Ceil  Duck Floor  Noise Gate  ...     │    │
 *  │  └─────────────────────────────────────────────┘    │
 *  └─────────────────────────────────────────────────────┘
 */
class LiveFlowAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit LiveFlowAudioProcessorEditor (LiveFlowAudioProcessor&);
    ~LiveFlowAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    static juce::String formatSignedDb (double value);
    static juce::String formatPercent (double value);
    static juce::String formatMilliseconds (double value);
    static juce::String formatDb (double value);

    std::unique_ptr<SliderAttachment> attachSlider (const juce::String& parameterId, juce::Slider& slider);

    LiveFlowAudioProcessor& processor;
    LiveFlowLookAndFeel lookAndFeel;

    // Main components
    VisualizerComponent visualizer;
    PresenceIndicator presenceIndicator;
    GainTimelineComponent gainTimeline;

    // Core parameter knobs
    CompactKnob balanceKnob;
    CompactKnob dynamicsKnob;
    juce::ToggleButton expansionModeToggle;
    CompactKnob speedKnob;
    CompactKnob rangeKnob;

    // Anchor control: toggle + knob
    juce::ToggleButton anchorAutoToggle;
    CompactKnob anchorKnob;

    // Header buttons
    juce::HyperlinkButton websiteLink;
    juce::TextButton expertButton;
    juce::TextButton helpButton;
    juce::TextButton aboutButton;
    juce::TextButton langButton;
    
    // Expert section
    juce::Component expertSection;
    bool expertVisible = false;

    CompactKnob boostCeilingKnob;
    CompactKnob duckFloorKnob;
    CompactKnob noiseGateKnob;
    CompactKnob lookAheadKnob;
    CompactKnob presenceReleaseKnob;

    // Attachments
    std::unique_ptr<SliderAttachment> balanceAttachment;
    std::unique_ptr<SliderAttachment> dynamicsAttachment;
    std::unique_ptr<ButtonAttachment> expansionModeAttachment;
    std::unique_ptr<SliderAttachment> speedAttachment;
    std::unique_ptr<SliderAttachment> rangeAttachment;
    std::unique_ptr<ButtonAttachment> anchorAutoAttachment;
    std::unique_ptr<SliderAttachment> anchorManualAttachment;
    std::unique_ptr<SliderAttachment> boostCeilingAttachment;
    std::unique_ptr<SliderAttachment> duckFloorAttachment;
    std::unique_ptr<SliderAttachment> noiseGateAttachment;
    std::unique_ptr<SliderAttachment> lookAheadAttachment;
    std::unique_ptr<SliderAttachment> presenceReleaseAttachment;
    
    // Overlays must be last to render top-most!
    HelpOverlay helpOverlay;
    AboutOverlay aboutOverlay;
    
    // Licensing
    LicenseManager licenseManager;
    ActivationOverlay activationOverlay { licenseManager };
    bool validationTriggered = false;
    
    bool isChinese = juce::SystemStats::getUserLanguage().startsWithIgnoreCase("zh");
    void updateAllTexts();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiveFlowAudioProcessorEditor)
};
} // namespace liveflow
