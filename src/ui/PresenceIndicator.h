#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace liveflow
{
/**
 * PresenceIndicator — a small arc-style voice presence meter.
 * Shows 0.0 (no voice) to 1.0 (full voice) as a sweeping arc fill.
 * FabFilter sidechain meter style.
 */
class PresenceIndicator final : public juce::Component
{
public:
    void setPresence (const float newPresence) noexcept
    {
        presence = juce::jlimit (0.0f, 1.0f, newPresence);
    }

    void paint (juce::Graphics& graphics) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background card
        graphics.setColour (juce::Colour (0x0cffffff));
        graphics.fillRoundedRectangle (bounds, 10.0f);
        graphics.setColour (juce::Colour (0x14ffffff));
        graphics.drawRoundedRectangle (bounds.reduced (0.5f), 10.0f, 1.0f);

        // Title
        auto titleArea = bounds.removeFromTop (14.0f);
        graphics.setColour (juce::Colour (0xff89a0ba));
        graphics.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        graphics.drawText ("VOICE", titleArea, juce::Justification::centred);

        // Arc meter
        const auto arcBounds = bounds.reduced (6.0f, 2.0f);
        const auto centreX = arcBounds.getCentreX();
        const auto centreY = arcBounds.getCentreY() + 4.0f;
        const auto radius = juce::jmin (arcBounds.getWidth(), arcBounds.getHeight()) * 0.42f;

        const auto startAngle = -juce::MathConstants<float>::pi * 0.75f;
        const auto endAngle = juce::MathConstants<float>::pi * 0.75f;
        const auto activeAngle = juce::jmap (presence, 0.0f, 1.0f, startAngle, endAngle);

        // Background arc
        juce::Path bgArc;
        bgArc.addCentredArc (centreX, centreY, radius, radius, 0.0f, startAngle, endAngle, true);
        graphics.setColour (juce::Colour (0x1affffff));
        graphics.strokePath (bgArc, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Active arc
        if (presence > 0.01f)
        {
            juce::Path activeArc;
            activeArc.addCentredArc (centreX, centreY, radius, radius, 0.0f, startAngle, activeAngle, true);

            const auto arcColour = juce::Colour (0xff56d4ff).interpolatedWith (juce::Colour (0xff3ecfd5), presence);
            juce::ColourGradient glow (arcColour.brighter (0.15f), centreX, centreY - radius,
                                       arcColour.darker (0.2f), centreX, centreY + radius, false);
            graphics.setGradientFill (glow);
            graphics.strokePath (activeArc, juce::PathStrokeType (5.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Glow dot at the tip of the arc
            const auto tipX = centreX + radius * std::cos (activeAngle - juce::MathConstants<float>::halfPi);
            const auto tipY = centreY + radius * std::sin (activeAngle - juce::MathConstants<float>::halfPi);
            graphics.setColour (arcColour.withAlpha (0.5f));
            graphics.fillEllipse (tipX - 4.0f, tipY - 4.0f, 8.0f, 8.0f);
            graphics.setColour (juce::Colour (0xfff3f7fb));
            graphics.fillEllipse (tipX - 2.5f, tipY - 2.5f, 5.0f, 5.0f);
        }

        // Value text
        auto valueArea = bounds.removeFromBottom (14.0f);
        const auto presenceColour = presence > 0.7f ? juce::Colour (0xff56d4ff)
                                  : presence > 0.3f ? juce::Colour (0xff89a0ba)
                                                    : juce::Colour (0xff5a7088);
        graphics.setColour (presenceColour);
        graphics.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        graphics.drawText (juce::String (presence, 2), valueArea, juce::Justification::centred);
    }

private:
    float presence = 0.0f;
};
} // namespace liveflow
