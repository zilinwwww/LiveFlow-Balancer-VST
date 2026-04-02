#include "AboutOverlay.h"
#include "Translations.h"

#ifndef LIVEFLOW_VERSION_EXT
#define LIVEFLOW_VERSION_EXT "dev"
#endif

namespace liveflow
{
AboutOverlay::AboutOverlay()
{
    addAndMakeVisible (closeButton);
    closeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x10ffffff));
    closeButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffeef5fb));
    closeButton.onClick = [this] { if (onClose) onClose(); };
}

juce::Rectangle<float> AboutOverlay::getCardBounds() const
{
    return getLocalBounds().toFloat().withSizeKeepingCentre (420.0f, 260.0f);
}

void AboutOverlay::paint (juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat();

    // Full-screen dark tint over existing plugin UI
    graphics.setColour (juce::Colour (0x80040910));
    graphics.fillRect (bounds);

    // Center card
    auto cardBounds = getCardBounds();

    juce::ColourGradient bg (juce::Colour (0xff0d192b), cardBounds.getTopLeft(),
                             juce::Colour (0xff070e1a), cardBounds.getBottomRight(), false);
    graphics.setGradientFill (bg);
    graphics.fillRoundedRectangle (cardBounds, 12.0f);

    // Card inner light border
    graphics.setColour (juce::Colour (0x2affffff));
    graphics.drawRoundedRectangle (cardBounds.reduced (0.5f), 12.0f, 1.0f);

    // Header area
    auto headerArea = cardBounds.removeFromTop (50.0f);
    const auto lang = currentIsChinese ? i18n::Language::Chinese : i18n::Language::English;

    graphics.setColour (juce::Colour (0xffffffff));
    graphics.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    graphics.drawText (i18n::getText ("About_Title", lang), headerArea, juce::Justification::centred);

    // Border line under header
    graphics.setColour (juce::Colour (0x20ffffff));
    graphics.drawHorizontalLine (juce::roundToInt (headerArea.getBottom()), cardBounds.getX(), cardBounds.getRight());

    // Content body
    auto contentArea = cardBounds.reduced (28.0f, 20.0f);

    // Description text
    graphics.setColour (juce::Colour (0xffeef5fb));
    graphics.setFont (juce::FontOptions (13.5f));

    auto aboutText = i18n::getText ("About_Desc", lang);

    juce::AttributedString attrStr;
    attrStr.append (aboutText, juce::FontOptions (13.5f), juce::Colour (0xffeef5fb));
    attrStr.setLineSpacing (4.0f);

    juce::TextLayout layout;
    layout.createLayout (attrStr, contentArea.getWidth());
    layout.draw (graphics, contentArea);

    // Version line below description
    auto versionArea = contentArea;
    versionArea.removeFromTop (layout.getHeight() + 12.0f);
    graphics.setColour (juce::Colour (0xff3ecfd5));
    graphics.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    graphics.drawText ("Version: " + juce::String (LIVEFLOW_VERSION_EXT),
                       versionArea.removeFromTop (20.0f), juce::Justification::centredLeft);

    // Copyright
    graphics.setColour (juce::Colour (0xff6b7f96));
    graphics.setFont (juce::FontOptions (11.0f));
    graphics.drawText ("MIT License  |  github.com/zilinwwww/LiveFlow-Balancer-VST",
                       versionArea.removeFromTop (18.0f), juce::Justification::centredLeft);
}

void AboutOverlay::resized()
{
    auto bounds = getLocalBounds();
    auto cardBounds = bounds.toFloat().withSizeKeepingCentre (420.0f, 260.0f);
    auto headerArea = cardBounds.removeFromTop (50.0f);

    closeButton.setBounds (juce::Rectangle<int> (
        juce::roundToInt (headerArea.getRight() - 50.0f),
        juce::roundToInt (headerArea.getY()),
        50, 50).reduced (10));
}

void AboutOverlay::mouseDown (const juce::MouseEvent& event)
{
    // Close overlay if clicking outside the card
    auto cardBounds = getCardBounds();
    if (! cardBounds.contains (event.position))
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
