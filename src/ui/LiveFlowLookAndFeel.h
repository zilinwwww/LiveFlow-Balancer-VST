#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace liveflow
{
class LiveFlowLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LiveFlowLookAndFeel();
    ~LiveFlowLookAndFeel() override = default;

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override;

    void drawRotarySlider (juce::Graphics&,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override;

    void drawToggleButton (juce::Graphics&,
                           juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawButtonBackground (juce::Graphics&,
                               juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

private:
    juce::Typeface::Ptr typeface;
};
} // namespace liveflow
