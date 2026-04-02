#include "LiveFlowLookAndFeel.h"

#include <BinaryData.h>

namespace liveflow
{
LiveFlowLookAndFeel::LiveFlowLookAndFeel()
{
    typeface = juce::Typeface::createSystemTypefaceFor (BinaryData::SpaceGroteskVariable_ttf,
                                                        BinaryData::SpaceGroteskVariable_ttfSize);

    setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff3ecfd5));
    setColour (juce::Slider::thumbColourId, juce::Colour (0xfffbf7ef));
    setColour (juce::Slider::trackColourId, juce::Colour (0x30ffffff));
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff16253d));
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xfff3f2ea));
}

juce::Typeface::Ptr LiveFlowLookAndFeel::getTypefaceForFont (const juce::Font& font)
{
    juce::ignoreUnused (font);
    return typeface != nullptr ? typeface : LookAndFeel_V4::getTypefaceForFont (font);
}

void LiveFlowLookAndFeel::drawRotarySlider (juce::Graphics& graphics,
                                            const int x,
                                            const int y,
                                            const int width,
                                            const int height,
                                            const float sliderPosProportional,
                                            const float rotaryStartAngle,
                                            const float rotaryEndAngle,
                                            juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y), static_cast<float> (width), static_cast<float> (height)).reduced (8.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = juce::jmap (sliderPosProportional, 0.0f, 1.0f, rotaryStartAngle, rotaryEndAngle);
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    
    // Outer Track Ring
    juce::Path outerRing;
    outerRing.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    graphics.setColour (juce::Colour (0x10ffffff));
    graphics.strokePath (outerRing, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Active Value Ring with Bloom/Glow
    juce::Path activeRing;
    activeRing.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
    
    // Outer glow for the active ring
    juce::ColourGradient bloom (accent.brighter (0.2f).withAlpha(0.6f), centre.x, centre.y - radius,
                                accent.withAlpha(0.0f), centre.x, centre.y, true);
    graphics.setGradientFill (bloom);
    graphics.strokePath (activeRing, juce::PathStrokeType (12.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Solid inner active ring
    juce::ColourGradient linearGlow (accent.brighter(0.2f), centre.x, centre.y - radius,
                                     accent.darker(0.3f), centre.x, centre.y + radius, false);
    graphics.setGradientFill (linearGlow);
    graphics.strokePath (activeRing, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Inner Knob Dome
    const auto innerBounds = bounds.reduced (16.0f);
    juce::ColourGradient domeGrad (juce::Colour (0xff1f2d40), innerBounds.getTopLeft(),
                                   juce::Colour (0xff0a1320), innerBounds.getBottomRight(), false);
    graphics.setGradientFill (domeGrad);
    graphics.fillEllipse (innerBounds);
    
    // Knob Dome Highlight (Bevel)
    graphics.setColour (juce::Colour (0x2affffff));
    graphics.drawEllipse (innerBounds.reduced (0.5f), 1.0f);

    // Pointer (Sleek indicator dot/line)
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -radius * 0.60f, 3.0f, radius * 0.35f, 1.5f);
    graphics.setColour (slider.isEnabled() ? juce::Colour (0xffffffff) : juce::Colour (0x50ffffff));
    graphics.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

void LiveFlowLookAndFeel::drawToggleButton (juce::Graphics& graphics,
                                            juce::ToggleButton& button,
                                            const bool shouldDrawButtonAsHighlighted,
                                            const bool shouldDrawButtonAsDown)
{
    const auto bg = button.findColour (juce::ToggleButton::tickColourId);
    const auto isActive = button.getToggleState();
    const auto bounds = button.getLocalBounds().toFloat();
    
    // Premium glowing pill card
    juce::Colour bgColour = isActive ? bg.withAlpha(0.2f) : juce::Colour(0x0cffffff);
    
    if (shouldDrawButtonAsDown) bgColour = bgColour.brighter (0.1f);
    else if (shouldDrawButtonAsHighlighted) bgColour = bgColour.brighter (0.05f);

    // Subtle drop shadow / bloom for active toggles
    if (isActive)
    {
        juce::ColourGradient bloom (bg.withAlpha(0.25f), bounds.getCentreX(), bounds.getCentreY(),
                                    bg.withAlpha(0.0f), bounds.getCentreX(), bounds.getY() - 5.0f, true);
        graphics.setGradientFill (bloom);
        graphics.fillRoundedRectangle (bounds.expanded(4.0f), 8.0f);
    }
        
    graphics.setColour (bgColour);
    graphics.fillRoundedRectangle (bounds, 5.0f);
    
    // Glossy Border
    juce::Colour borderColour = isActive ? bg.withAlpha(0.6f) : juce::Colour(0x20ffffff);
    graphics.setColour (borderColour);
    graphics.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);

    // Text Label with glow
    graphics.setColour (isActive ? juce::Colour(0xffffffff) : button.findColour (juce::ToggleButton::textColourId));
    graphics.setFont (juce::FontOptions (10.0f, isActive ? juce::Font::bold : juce::Font::plain));
    
    if (isActive)
    {
        graphics.setColour (bg.withAlpha(0.5f));
        graphics.drawText (button.getButtonText(), bounds.translated(0, 1), juce::Justification::centred);
        graphics.setColour (juce::Colour(0xffffffff));
    }
    graphics.drawText (button.getButtonText(), bounds, juce::Justification::centred);
}

void LiveFlowLookAndFeel::drawButtonBackground (juce::Graphics& graphics,
                                                juce::Button& button,
                                                const juce::Colour& backgroundColour,
                                                const bool shouldDrawButtonAsHighlighted,
                                                const bool shouldDrawButtonAsDown)
{
    auto base = backgroundColour;
    if (shouldDrawButtonAsDown)
        base = base.brighter (0.15f);
    else if (shouldDrawButtonAsHighlighted)
        base = base.brighter (0.08f);

    const auto bounds = button.getLocalBounds().toFloat();
    
    // Modern gradient
    juce::ColourGradient gradient (base.brighter (0.05f), bounds.getTopLeft(),
                                   base.darker (0.25f), bounds.getBottomRight(), false);

    graphics.setGradientFill (gradient);
    graphics.fillRoundedRectangle (bounds, 18.0f);

    // Bright top edge bevel
    graphics.setColour (juce::Colour (0x30ffffff));
    graphics.drawRoundedRectangle (bounds.reduced (0.5f), 18.0f, 1.0f);
}
} // namespace liveflow
