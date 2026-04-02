#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace liveflow
{
/**
 * CompactKnob — a small rotary knob with label above and value below.
 * Replaces the old ParameterDial (horizontal slider). Designed for the
 * bottom parameter bar in the FabFilter-style layout.
 *
 * Layout (vertical):
 *   ┌─────────────┐
 *   │  LABEL      │  ← small caps label
 *   │   (knob)    │  ← rotary slider
 *   │  +3.0 dB    │  ← value text (click-to-edit)
 *   └─────────────┘
 */
class CompactKnob final : public juce::Component
{
public:
    using ValueFormatter = std::function<juce::String (double)>;

    CompactKnob (juce::String labelToUse,
                 juce::Colour accentColour,
                 ValueFormatter formatterToUse)
        : label (std::move (labelToUse)),
          accent (accentColour),
          formatter (std::move (formatterToUse))
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xfff3f7fb));
        slider.onValueChange = [this] { refreshValueText(); repaint(); };
        addAndMakeVisible (slider);

        valueEditor.setJustificationType (juce::Justification::centred);
        valueEditor.setColour (juce::Label::textColourId, juce::Colour (0xffeef5fb));
        valueEditor.setColour (juce::Label::textWhenEditingColourId, juce::Colour (0xffeef5fb));
        valueEditor.setColour (juce::Label::backgroundWhenEditingColourId, juce::Colour (0xff1a2a42));
        valueEditor.setColour (juce::Label::outlineWhenEditingColourId, accent.withAlpha (0.6f));
        valueEditor.setFont (juce::FontOptions (11.5f, juce::Font::bold));
        valueEditor.setEditable (false, true, false);
        valueEditor.onTextChange = [this]
        {
            const auto text = valueEditor.getText();
            const auto numericValue = text.retainCharacters ("0123456789.+-").getDoubleValue();
            slider.setValue (numericValue, juce::sendNotification);
        };
        addAndMakeVisible (valueEditor);

        refreshValueText();
    }

    [[nodiscard]] juce::Slider& getSlider() noexcept { return slider; }

    void setLabel (const juce::String& newLabel)
    {
        label = newLabel;
        repaint();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const auto labelHeight = 14;
        const auto valueHeight = 16;

        bounds.removeFromTop (labelHeight);
        auto valueBounds = bounds.removeFromBottom (valueHeight);
        valueEditor.setBounds (valueBounds.reduced (2, 0));

        // The remaining central area is for the knob
        auto knobBounds = bounds.reduced (4, 0);
        slider.setBounds (knobBounds);
    }

    void paint (juce::Graphics& graphics) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Subtle background card
        graphics.setColour (juce::Colour (0x0cffffff));
        graphics.fillRoundedRectangle (bounds, 10.0f);
        graphics.setColour (juce::Colour (0x14ffffff));
        graphics.drawRoundedRectangle (bounds.reduced (0.5f), 10.0f, 1.0f);

        // Label at top
        auto labelBounds = bounds.removeFromTop (14.0f).reduced (4.0f, 0.0f);
        graphics.setColour (juce::Colour (0xff89a0ba));
        graphics.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        graphics.drawText (label.toUpperCase(), labelBounds, juce::Justification::centred);
    }

private:
    void refreshValueText()
    {
        if (formatter)
            valueEditor.setText (formatter (slider.getValue()), juce::dontSendNotification);
        else
            valueEditor.setText (juce::String (slider.getValue(), 1), juce::dontSendNotification);
    }

    juce::Slider slider;
    juce::Label valueEditor;
    juce::String label;
    juce::Colour accent;
    ValueFormatter formatter;
};
} // namespace liveflow
