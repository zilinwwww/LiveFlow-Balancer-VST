#pragma once

#include <array>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/VisualizationState.h"

namespace liveflow
{
/**
 * GainTimelineComponent — a horizontal strip showing gain history over time.
 * 0 dB centred, green above (boosting), orange/red below (ducking).
 * Voice presence is shown as a semi-transparent blue background band.
 *
 * Similar to FabFilter Pro-C's gain reduction meter.
 */
class GainTimelineComponent final : public juce::Component
{
public:
    void updateFromSnapshot (const VisualizationState::Snapshot& snapshot) noexcept
    {
        gainHistory = snapshot.gainHistory;
        gainHeadIndex = snapshot.gainHeadIndex;
        currentGainDb = snapshot.gainDb;
        voicePresence = snapshot.voicePresence;
    }

    void paint (juce::Graphics& graphics) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        graphics.setColour (juce::Colour (0x0cffffff));
        graphics.fillRoundedRectangle (bounds, 8.0f);

        // Current gain value readout on the right
        auto readout = bounds.removeFromRight (38.0f).reduced (2.0f, 4.0f);

        const auto plotBounds = bounds.reduced (2.0f, 3.0f);
        const auto centreY = plotBounds.getCentreY();
        const auto maxGainDisplay = 12.0f;
        const auto historyCount = static_cast<int> (gainHistory.size());

        // Draw voice presence background fade
        const auto presenceAlpha = juce::jlimit (0.0f, 0.18f, voicePresence * 0.18f);
        graphics.setColour (juce::Colour (0xff56d4ff).withAlpha (presenceAlpha));
        graphics.fillRoundedRectangle (plotBounds, 6.0f);

        // Draw 0 dB center line
        graphics.setColour (juce::Colour (0x20ffffff));
        graphics.drawHorizontalLine (juce::roundToInt (centreY), plotBounds.getX(), plotBounds.getRight());

        // Draw gain history as filled area
        juce::Path boostPath;
        juce::Path duckPath;
        bool boostStarted = false;
        bool duckStarted = false;

        for (int i = 0; i < historyCount; ++i)
        {
            const auto orderedIndex = (gainHeadIndex + i) % historyCount;
            const auto gainDb = gainHistory[static_cast<size_t> (orderedIndex)];
            const auto x = juce::jmap (static_cast<float> (i), 0.0f, static_cast<float> (historyCount - 1),
                                       plotBounds.getX(), plotBounds.getRight());
            const auto normGain = juce::jlimit (-maxGainDisplay, maxGainDisplay, gainDb) / maxGainDisplay;
            const auto halfHeight = plotBounds.getHeight() * 0.48f;

            if (gainDb >= 0.0f)
            {
                const auto y = centreY - halfHeight * normGain;
                if (! boostStarted)
                {
                    boostPath.startNewSubPath (x, centreY);
                    boostStarted = true;
                }
                boostPath.lineTo (x, y);
            }

            if (gainDb <= 0.0f)
            {
                const auto y = centreY - halfHeight * normGain; // normGain is negative, so y > centreY
                if (! duckStarted)
                {
                    duckPath.startNewSubPath (x, centreY);
                    duckStarted = true;
                }
                duckPath.lineTo (x, y);
            }
        }

        if (boostStarted)
        {
            boostPath.lineTo (plotBounds.getRight(), centreY);
            boostPath.closeSubPath();
            graphics.setColour (juce::Colour (0xff81e69d).withAlpha (0.35f));
            graphics.fillPath (boostPath);
            graphics.setColour (juce::Colour (0xff81e69d).withAlpha (0.7f));
            graphics.strokePath (boostPath, juce::PathStrokeType (1.0f));
        }

        if (duckStarted)
        {
            duckPath.lineTo (plotBounds.getRight(), centreY);
            duckPath.closeSubPath();
            graphics.setColour (juce::Colour (0xffffaf6e).withAlpha (0.35f));
            graphics.fillPath (duckPath);
            graphics.setColour (juce::Colour (0xffffaf6e).withAlpha (0.7f));
            graphics.strokePath (duckPath, juce::PathStrokeType (1.0f));
        }

        const auto gainColour = currentGainDb < -0.1f ? juce::Colour (0xffffaf6e)
                              : currentGainDb > 0.1f  ? juce::Colour (0xff81e69d)
                                                      : juce::Colour (0xff89a0ba);
        graphics.setColour (gainColour);
        graphics.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        const auto prefix = currentGainDb > 0.05f ? "+" : "";
        graphics.drawText (prefix + juce::String (currentGainDb, 1),
                           readout, juce::Justification::centred);
    }

private:
    std::array<float, VisualizationState::gainHistorySize> gainHistory {};
    int gainHeadIndex = 0;
    float currentGainDb = 0.0f;
    float voicePresence = 0.0f;
};
} // namespace liveflow
