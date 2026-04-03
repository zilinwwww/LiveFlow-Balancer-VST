#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "licensing/LicenseManager.h"

namespace liveflow
{
/**
 * ActivationOverlay — Fullscreen overlay shown when the plugin is not licensed.
 * Contains: title, subtitle, key input, activate button, status message.
 * Styled to match LiveFlowLookAndFeel dark theme.
 */
class ActivationOverlay final : public juce::Component
{
public:
    ActivationOverlay (LicenseManager& licenseManagerRef)
        : licenseManager (licenseManagerRef)
    {
        title.setText ("LiveFlow Balancer", juce::dontSendNotification);
        title.setFont (juce::FontOptions (24.0f, juce::Font::bold));
        title.setColour (juce::Label::textColourId, juce::Colour (0xffeff5fb));
        title.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (title);

        subtitle.setText ("Enter your license key to activate", juce::dontSendNotification);
        subtitle.setFont (juce::FontOptions (13.0f));
        subtitle.setColour (juce::Label::textColourId, juce::Colour (0xff89a0ba));
        subtitle.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (subtitle);

        keyInput.setMultiLine (false);
        keyInput.setReturnKeyStartsNewLine (false);
        keyInput.setFont (juce::FontOptions (16.0f));
        keyInput.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff111c2d));
        keyInput.setColour (juce::TextEditor::textColourId, juce::Colour (0xff3ecfd5));
        keyInput.setColour (juce::TextEditor::outlineColourId, juce::Colour (0x20ffffff));
        keyInput.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xff3ecfd5));
        keyInput.setTextToShowWhenEmpty ("LF-XXXX-XXXX-XXXX-XXXX", juce::Colour (0xff5a7088));
        keyInput.setJustification (juce::Justification::centred);
        addAndMakeVisible (keyInput);

        activateButton.setButtonText ("Activate");
        activateButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3ecfd5));
        activateButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff050d18));
        activateButton.onClick = [this] { onActivateClicked(); };
        addAndMakeVisible (activateButton);

        statusLabel.setText ("", juce::dontSendNotification);
        statusLabel.setFont (juce::FontOptions (12.0f));
        statusLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (statusLabel);

        // Show existing status message if any
        updateStatus();
    }

    void paint (juce::Graphics& g) override
    {
        // Full screen dark overlay
        g.fillAll (juce::Colour (0xff09121e));

        // Center card
        auto card = getLocalBounds().withSizeKeepingCentre (400, 280).toFloat();
        juce::ColourGradient bg (juce::Colour (0xff0d192b), card.getTopLeft(),
                                  juce::Colour (0xff070e1a), card.getBottomRight(), false);
        g.setGradientFill (bg);
        g.fillRoundedRectangle (card, 14.0f);

        g.setColour (juce::Colour (0x28ffffff));
        g.drawRoundedRectangle (card.reduced (0.5f), 14.0f, 1.0f);
    }

    void resized() override
    {
        auto card = getLocalBounds().withSizeKeepingCentre (400, 280);
        auto inner = card.reduced (30, 24);

        title.setBounds (inner.removeFromTop (32));
        inner.removeFromTop (4);
        subtitle.setBounds (inner.removeFromTop (22));
        inner.removeFromTop (20);
        keyInput.setBounds (inner.removeFromTop (36).reduced (10, 0));
        inner.removeFromTop (14);
        activateButton.setBounds (inner.removeFromTop (36).reduced (80, 0));
        inner.removeFromTop (12);
        statusLabel.setBounds (inner.removeFromTop (40));
    }

    /** Called from the editor to refresh the overlay state. */
    void updateStatus()
    {
        auto msg = licenseManager.getStatusMessage();
        statusLabel.setText (msg, juce::dontSendNotification);
        auto col = msg.containsIgnoreCase ("success") ? juce::Colour (0xff81e69d) : juce::Colour (0xffff6b6b);
        statusLabel.setColour (juce::Label::textColourId, msg.isEmpty() ? juce::Colours::transparentBlack : col);
    }

    std::function<void()> onActivated;

private:
    LicenseManager& licenseManager;
    juce::Label title, subtitle, statusLabel;
    juce::TextEditor keyInput;
    juce::TextButton activateButton;

    void onActivateClicked()
    {
        auto key = keyInput.getText().trim().toUpperCase();
        if (key.isEmpty()) return;

        activateButton.setEnabled (false);
        statusLabel.setText ("Activating...", juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xff89a0ba));

        // Run activation on a background thread
        auto* mgr = &licenseManager;
        auto weakThis = juce::Component::SafePointer<ActivationOverlay> (this);

        juce::Thread::launch ([mgr, key, weakThis]
        {
            bool success = mgr->activate (key);

            juce::MessageManager::callAsync ([weakThis, success]
            {
                if (weakThis == nullptr) return;
                weakThis->activateButton.setEnabled (true);
                weakThis->updateStatus();
                if (success && weakThis->onActivated)
                    weakThis->onActivated();
            });
        });
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ActivationOverlay)
};
} // namespace liveflow
