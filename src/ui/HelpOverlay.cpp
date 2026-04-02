#include "HelpOverlay.h"
#include "Translations.h"

namespace liveflow
{
HelpOverlay::HelpContent::HelpContent()
{
    populateSections (false);
}

void HelpOverlay::HelpContent::populateSections (bool isChinese)
{
    const auto lang = isChinese ? i18n::Language::Chinese : i18n::Language::English;
    
    sections.clear();
    auto defs = i18n::getHelpSections (lang);
    for (const auto& d : defs)
    {
        sections.push_back ({ d.title, d.description });
    }

    // Approximate a proper height for content layout
    int totalHeight = 30 + static_cast<int>(sections.size()) * 94; // Top margin + sections * (Title + spacing + generous wrap)
    
    setBounds (0, 0, 500, totalHeight);
    repaint();
}

void HelpOverlay::HelpContent::paint (juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat().reduced (24.0f, 30.0f);

    for (const auto& sec : sections)
    {
        // Draw Title
        graphics.setColour (juce::Colour (0xff3ecfd5)); // Cyan Glow Accent
        graphics.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        graphics.drawText (sec.title, bounds.removeFromTop (24.0f), juce::Justification::topLeft);
        
        // Draw Description
        bounds.removeFromTop (4.0f);
        juce::Colour bodyColor (0xffeef5fb);
        juce::Font bodyFont (juce::FontOptions (13.0f, juce::Font::plain));

        juce::AttributedString attributedString (sec.description);
        attributedString.setColour (bodyColor);
        attributedString.setFont (bodyFont);
        attributedString.setLineSpacing (3.0f);
        
        juce::TextLayout layout;
        layout.createLayout (attributedString, bounds.getWidth());
        
        layout.draw (graphics, juce::Rectangle<float> (bounds.getX(), bounds.getY(), bounds.getWidth(), layout.getHeight()));
        
        bounds.removeFromTop (layout.getHeight() + 24.0f);
    }
}

HelpOverlay::HelpOverlay()
{
    addAndMakeVisible (viewport);
    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (true, false, true, false);
    viewport.setScrollBarThickness (8);

    addAndMakeVisible (closeButton);
    closeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x10ffffff));
    closeButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffeef5fb));
    closeButton.onClick = [this] { if (onClose) onClose(); };
}

void HelpOverlay::paint (juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Full-screen Dark Tint over existing plugin UI
    graphics.setColour (juce::Colour (0x80040910));
    graphics.fillRect (bounds);

    // 2. Center Card Frame
    auto cardBounds = bounds.withSizeKeepingCentre (580.0f, 440.0f);
    
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
    graphics.drawText (i18n::getText ("Header_Title", lang), headerArea, juce::Justification::centred);

    // Border line under header
    graphics.setColour (juce::Colour (0x20ffffff));
    graphics.drawHorizontalLine (juce::roundToInt (headerArea.getBottom()), cardBounds.getX(), cardBounds.getRight());
}

void HelpOverlay::resized()
{
    auto bounds = getLocalBounds();
    auto cardBounds = bounds.withSizeKeepingCentre (580, 440);

    auto headerArea = cardBounds.removeFromTop (50);
    closeButton.setBounds (headerArea.removeFromRight (50).reduced (10));

    viewport.setBounds (cardBounds.reduced (4));
    
    // Expand or shrink content view to match width
    content.setBounds (0, 0, viewport.getMaximumVisibleWidth(), content.getHeight());
}

void HelpOverlay::mouseDown (const juce::MouseEvent& event)
{
    // Close overlay if clicking outside the card
    auto bounds = getLocalBounds();
    auto cardBounds = bounds.withSizeKeepingCentre (580, 440);
    if (! cardBounds.contains (event.position.toInt()))
    {
        if (onClose) onClose();
    }
}
void HelpOverlay::updateLanguage (bool isChinese)
{
    currentIsChinese = isChinese;
    content.populateSections (isChinese);
    repaint();
}

} // namespace liveflow
