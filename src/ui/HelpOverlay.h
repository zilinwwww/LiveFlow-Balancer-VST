#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace liveflow
{
class HelpOverlay final : public juce::Component
{
public:
    HelpOverlay();
    ~HelpOverlay() override = default;

    void paint (juce::Graphics& graphics) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;

    void updateLanguage (bool isChinese);

    std::function<void()> onClose;

private:
    class HelpSection
    {
    public:
        juce::String title;
        juce::String description;
    };

    class HelpContent : public juce::Component
    {
    public:
        HelpContent();
        void paint (juce::Graphics& graphics) override;
        void populateSections (bool isChinese);

    private:
        std::vector<HelpSection> sections;
    };

    juce::TextButton closeButton { "X" };
    juce::Viewport viewport;
    HelpContent content;
    bool currentIsChinese = false;
};
} // namespace liveflow
