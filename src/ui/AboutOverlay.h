#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace liveflow
{
class AboutOverlay final : public juce::Component
{
public:
    AboutOverlay();
    ~AboutOverlay() override = default;

    void paint (juce::Graphics& graphics) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;

    void updateLanguage (bool isChinese);

    std::function<void()> onClose;

private:
    juce::TextButton closeButton { "X" };
    juce::HyperlinkButton websiteLink;
    bool currentIsChinese = false;
};
} // namespace liveflow
