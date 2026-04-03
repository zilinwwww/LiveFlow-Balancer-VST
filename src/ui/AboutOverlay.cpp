#include "AboutOverlay.h"
#include "Translations.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace liveflow
{
AboutOverlay::AboutOverlay()
{
    addAndMakeVisible (closeButton);
    closeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x10ffffff));
    closeButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffeef5fb));
    closeButton.onClick = [this] { if (onClose) onClose(); };

    addAndMakeVisible (websiteLink);
    websiteLink.setButtonText ("liveflow.micro-grav.com/balancer");
    websiteLink.setURL (juce::URL ("https://liveflow.micro-grav.com/balancer"));
    websiteLink.setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff3ecfd5));
    websiteLink.setJustificationType (juce::Justification::centred);
}

void AboutOverlay::paint (juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Full-screen Dark Tint
    graphics.setColour (juce::Colour (0x80040910));
    graphics.fillRect (bounds);

    // 2. Center Card Frame
    auto cardBounds = bounds.withSizeKeepingCentre (500.0f, 320.0f);
    
    juce::ColourGradient bg (juce::Colour (0xff0d192b), cardBounds.getTopLeft(),
                             juce::Colour (0xff070e1a), cardBounds.getBottomRight(), false);
    graphics.setGradientFill (bg);
    graphics.fillRoundedRectangle (cardBounds, 12.0f);

    // Card Inner Light Border
    graphics.setColour (juce::Colour (0x2affffff));
    graphics.drawRoundedRectangle (cardBounds.reduced (0.5f), 12.0f, 1.0f);

    // Header Title
    auto headerArea = cardBounds.removeFromTop (50.0f);
    graphics.setColour (juce::Colour (0xffffffff));
    graphics.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    
    const auto lang = currentIsChinese ? i18n::Language::Chinese : i18n::Language::English;
    graphics.drawText (i18n::getText ("About_Title", lang), headerArea, juce::Justification::centred);

    // Border line under header
    graphics.setColour (juce::Colour (0x20ffffff));
    graphics.drawHorizontalLine (juce::roundToInt (headerArea.getBottom()), cardBounds.getX(), cardBounds.getRight());

    // Body content
    auto bodyArea = cardBounds.reduced (30.0f);
    bodyArea.removeFromTop(20.0f); // Spacing
    
    // Draw Logo/Name
    graphics.setColour (juce::Colour (0xff3ecfd5));
    graphics.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    graphics.drawText ("LiveFlow Balancer", bodyArea.removeFromTop(40.0f), juce::Justification::centred);
    
    graphics.setColour (juce::Colour (0xffeef5fb));
    graphics.setFont (juce::FontOptions (14.0f, juce::Font::plain));
#ifdef LIVEFLOW_VERSION_EXT
    juce::String version = LIVEFLOW_VERSION_EXT;
#else
    juce::String version = "v0.1.0";
#endif
    juce::String desc = i18n::getText ("About_Desc", lang) + "\n\nVersion: " + version + "\n\nDeveloped by Micro-Grav Studio.";
    graphics.drawMultiLineText (desc, juce::roundToInt(bodyArea.getX()), juce::roundToInt(bodyArea.getY() + 10), juce::roundToInt(bodyArea.getWidth()), juce::Justification::centred);
}

void AboutOverlay::resized()
{
    auto bounds = getLocalBounds();
    auto cardBounds = bounds.withSizeKeepingCentre (500, 320);

    auto headerArea = cardBounds.removeFromTop (50);
    closeButton.setBounds (headerArea.removeFromRight (50).reduced (10));

    // Place the link at the bottom of the card
    auto linkArea = cardBounds.removeFromBottom (50);
    websiteLink.setBounds (linkArea);
}

void AboutOverlay::mouseDown (const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds();
    auto cardBounds = bounds.withSizeKeepingCentre (500, 320);
    if (! cardBounds.contains (event.position.toInt()))
    {
        if (onClose) onClose();
    }
}

void AboutOverlay::updateLanguage (bool isChinese)
{
    currentIsChinese = isChinese;
    repaint();
}

} // namespace liveflow
