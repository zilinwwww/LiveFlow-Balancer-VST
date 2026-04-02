#include "LiveFlowAudioProcessorEditor.h"

#include "LiveFlowAudioProcessor.h"

namespace
{
constexpr int defaultEditorWidth = 880;
constexpr int defaultEditorHeight = 600;

juce::Colour coreAccent (const int index)
{
    static const std::array colours {
        juce::Colour (0xff3ecfd5), // Balance — cyan
        juce::Colour (0xffffb76d), // Dynamics — amber
        juce::Colour (0xff8ae4a9), // Speed — green
        juce::Colour (0xff9fcfff), // Range — light blue
        juce::Colour (0xff56d4ff)  // Anchor — blue
    };

    return colours[static_cast<size_t> (index % static_cast<int> (colours.size()))];
}

juce::Colour expertAccent (const int index)
{
    static const std::array colours {
        juce::Colour (0xff79e28e), // Boost Ceiling — green
        juce::Colour (0xffff9d68), // Duck Floor — orange
        juce::Colour (0xff7fd8e6), // Noise Gate — teal
        juce::Colour (0xfff7d57a), // Look-ahead — gold
        juce::Colour (0xff9ce8c8)  // Presence Release — mint
    };

    return colours[static_cast<size_t> (index % static_cast<int> (colours.size()))];
}
} // namespace

namespace liveflow
{
LiveFlowAudioProcessorEditor::LiveFlowAudioProcessorEditor (LiveFlowAudioProcessor& processorToEdit)
    : juce::AudioProcessorEditor (&processorToEdit),
      processor (processorToEdit),
      lookAndFeel(),
      visualizer (processorToEdit.getVisualizationState(), processorToEdit.getValueTreeState()),
      balanceKnob ("Balance", coreAccent (0),
                    [] (double v) { return formatSignedDb (v); }),
      dynamicsKnob ("Dynamics", coreAccent (1),
                     [] (double v) { return formatPercent (v); }),
      speedKnob ("Speed", coreAccent (2),
                  [] (double v) { return formatMilliseconds (v); }),
      rangeKnob ("Range", coreAccent (3),
                  [] (double v) { return juce::String (v, 1) + " dB"; }),
      anchorKnob ("Anchor", coreAccent (4),
                   [] (double v) { return formatDb (v); }),
      boostCeilingKnob ("Boost Ceil", expertAccent (0),
                         [] (double v) { return juce::String (v, 1) + " dB"; }),
      duckFloorKnob ("Duck Floor", expertAccent (1),
                      [] (double v) { return juce::String (v, 1) + " dB"; }),
      noiseGateKnob ("Noise Gate", expertAccent (2),
                      [] (double v) { return formatDb (v); }),
      lookAheadKnob ("Look-ahead", expertAccent (3),
                      [] (double v) { return formatMilliseconds (v); }),
      presenceReleaseKnob ("Pres. Rel.", expertAccent (4),
                            [] (double v) { return formatMilliseconds (v); })
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);
    setResizable (true, true);
    setResizeLimits (760, 520, 1200, 800);
    setSize (defaultEditorWidth, defaultEditorHeight);

    // Main components
    addAndMakeVisible (visualizer);
    addAndMakeVisible (presenceIndicator);
    addAndMakeVisible (gainTimeline);

    // Core knobs
    for (auto* knob : { &balanceKnob, &dynamicsKnob, &speedKnob, &rangeKnob, &anchorKnob })
        addAndMakeVisible (*knob);

    // Expansion mode toggle
    expansionModeToggle.setButtonText ("Compress");
    expansionModeToggle.setColour (juce::ToggleButton::textColourId, juce::Colour (0xff89a0ba));
    expansionModeToggle.setColour (juce::ToggleButton::tickColourId, coreAccent (1));
    expansionModeToggle.onClick = [this]
    {
        const auto lang = isChinese ? i18n::Language::Chinese : i18n::Language::English;
        expansionModeToggle.setButtonText (i18n::getText (expansionModeToggle.getToggleState() ? "Toggle_Expand" : "Toggle_Compress", lang));
        repaint();
    };
    addAndMakeVisible (expansionModeToggle);

    // Anchor auto toggle
    anchorAutoToggle.setButtonText ("Auto");
    anchorAutoToggle.setColour (juce::ToggleButton::textColourId, juce::Colour (0xff89a0ba));
    anchorAutoToggle.setColour (juce::ToggleButton::tickColourId, coreAccent (4));
    anchorAutoToggle.onClick = [this]
    {
        anchorKnob.getSlider().setEnabled (! anchorAutoToggle.getToggleState());
        repaint();
    };
    addAndMakeVisible (anchorAutoToggle);

    // Header buttons
    addAndMakeVisible (expertButton);
    expertButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x10ffffff));
    expertButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffeef4fb));
    expertButton.setButtonText ("Expert");
    expertButton.onClick = [this]
    {
        expertVisible = ! expertVisible;
        expertSection.setVisible (expertVisible);
        resized();
        repaint();
    };

    addAndMakeVisible (helpButton);
    helpButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x10ffffff));
    helpButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffeef4fb));
    helpButton.setButtonText ("Help");
    helpButton.onClick = [this]
    {
        helpOverlay.setVisible (true);
    };

    addAndMakeVisible (aboutButton);
    aboutButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x10ffffff));
    aboutButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffeef4fb));
    aboutButton.setButtonText ("About");
    aboutButton.onClick = [this]
    {
        aboutOverlay.setBounds (getLocalBounds());
        aboutOverlay.toFront (true);
        aboutOverlay.setVisible (true);
    };

    addAndMakeVisible (langButton);
    langButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x10ffffff));
    langButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffeef4fb));
    langButton.onClick = [this]
    {
        isChinese = ! isChinese;
        updateAllTexts();
    };

    addAndMakeVisible (expertSection);
    for (auto* knob : { &boostCeilingKnob, &duckFloorKnob, &noiseGateKnob, &lookAheadKnob, &presenceReleaseKnob })
        expertSection.addAndMakeVisible (*knob);

    expertSection.setVisible (expertVisible);

    // Parameter attachments
    balanceAttachment = attachSlider (param::balance, balanceKnob.getSlider());
    dynamicsAttachment = attachSlider (param::dynamics, dynamicsKnob.getSlider());
    expansionModeAttachment = std::make_unique<ButtonAttachment> (processor.getValueTreeState(), param::expansionMode, expansionModeToggle);
    speedAttachment = attachSlider (param::speed, speedKnob.getSlider());
    rangeAttachment = attachSlider (param::range, rangeKnob.getSlider());
    anchorManualAttachment = attachSlider (param::anchorManual, anchorKnob.getSlider());
    anchorAutoAttachment = std::make_unique<ButtonAttachment> (processor.getValueTreeState(), param::anchorAuto, anchorAutoToggle);

    boostCeilingAttachment = attachSlider (param::boostCeiling, boostCeilingKnob.getSlider());
    duckFloorAttachment = attachSlider (param::duckFloor, duckFloorKnob.getSlider());
    noiseGateAttachment = attachSlider (param::noiseGate, noiseGateKnob.getSlider());
    lookAheadAttachment = attachSlider (param::lookAhead, lookAheadKnob.getSlider());
    presenceReleaseAttachment = attachSlider (param::presenceRelease, presenceReleaseKnob.getSlider());

    // Initial state
    anchorKnob.getSlider().setEnabled (! anchorAutoToggle.getToggleState());

    addChildComponent (helpOverlay);
    helpOverlay.setVisible (false);
    helpOverlay.onClose = [this] { helpOverlay.setVisible (false); };

    addChildComponent (aboutOverlay);
    aboutOverlay.setVisible (false);
    aboutOverlay.onClose = [this] { aboutOverlay.setVisible (false); };

    updateAllTexts();

    startTimerHz (30);
    setSize (880, 600);
}

LiveFlowAudioProcessorEditor::~LiveFlowAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void LiveFlowAudioProcessorEditor::timerCallback()
{
    const auto snap = processor.getVisualizationState().capture();
    gainTimeline.updateFromSnapshot (snap);
    presenceIndicator.setPresence (snap.voicePresence);
    gainTimeline.repaint();
    presenceIndicator.repaint();
}

void LiveFlowAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    const auto bounds = getLocalBounds().toFloat();

    // Background gradient
    juce::ColourGradient background (juce::Colour (0xff09121e), bounds.getTopLeft(),
                                     juce::Colour (0xff111c2d), bounds.getBottomRight(), false);
    graphics.setGradientFill (background);
    graphics.fillRect (bounds);

    // Subtle horizontal line pattern
    graphics.setColour (juce::Colour (0x06ffffff));
    for (int line = 0; line < 8; ++line)
    {
        const auto y = juce::jmap (static_cast<float> (line), 0.0f, 7.0f, 0.0f, bounds.getBottom());
        graphics.drawHorizontalLine (juce::roundToInt (y), 0.0f, bounds.getRight());
    }

    // Title bar
    auto titleArea = getLocalBounds().reduced (14).removeFromTop (28);
    graphics.setColour (juce::Colour (0xffeff5fb));
    graphics.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    const auto title = juce::String("LiveFlow Balancer");
    auto titleWidth = graphics.getCurrentFont().getStringWidthFloat (title) + 8.0f;
    graphics.drawText (title, titleArea.removeFromLeft (static_cast<int>(titleWidth)), juce::Justification::centredLeft);

    graphics.setColour (juce::Colour (0x88eff5fb));
    graphics.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    const auto versionStr = juce::String (LIVEFLOW_VERSION_EXT);
    auto versionWidth = graphics.getCurrentFont().getStringWidthFloat (versionStr) + 8.0f;
    graphics.drawText (versionStr, titleArea.removeFromLeft (static_cast<int> (versionWidth)), juce::Justification::centredLeft);
}

void LiveFlowAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (14);
    auto titleArea = bounds.removeFromTop (32); // Title bar
    
    // Position header buttons
    auto buttonArea = titleArea.removeFromRight (190).reduced (0, 4);
    const auto btnGap = 5;
    const auto btnW = (buttonArea.getWidth() - 2 * btnGap) / 3;
    expertButton.setBounds (buttonArea.removeFromLeft (btnW));
    buttonArea.removeFromLeft (btnGap);
    helpButton.setBounds (buttonArea.removeFromLeft (btnW));
    buttonArea.removeFromLeft (btnGap);
    aboutButton.setBounds (buttonArea.removeFromLeft (btnW));

    titleArea.removeFromRight (20); // Spacing between lang toggle and header buttons
    auto langArea = titleArea.removeFromRight (60).reduced (0, 4);
    langButton.setBounds (langArea);

    // Calculate fixed height needed below the canvas (Removed expertButton bottom row)
    int fixedBottomHeight = 6 + 52 + 6 + 72; // Margins + middleStrip + coreKnobs
    if (expertVisible)
        fixedBottomHeight += 4 + 72; // Margin + expertSection

    // ── Main canvas ──
    const auto canvasHeight = juce::jmax (180, bounds.getHeight() - fixedBottomHeight);
    visualizer.setBounds (bounds.removeFromTop (canvasHeight));

    bounds.removeFromTop (6);

    // ── Middle strip: Presence indicator + Gain timeline ──
    auto middleStrip = bounds.removeFromTop (52);
    presenceIndicator.setBounds (middleStrip.removeFromLeft (64));
    middleStrip.removeFromLeft (6);
    gainTimeline.setBounds (middleStrip);

    bounds.removeFromTop (6);

    // ── Core parameter knobs ──
    auto knobRow = bounds.removeFromTop (72);
    const auto knobCount = 5;
    const auto knobGap = 6;
    const auto knobWidth = (knobRow.getWidth() - (knobCount - 1) * knobGap) / knobCount;

    balanceKnob.setBounds (knobRow.removeFromLeft (knobWidth));
    knobRow.removeFromLeft (knobGap);
    
    // Dynamics knob with Expansion toggle overlay
    auto dynArea = knobRow.removeFromLeft (knobWidth);
    dynamicsKnob.setBounds (dynArea);
    expansionModeToggle.setBounds (dynArea.removeFromTop (16).removeFromRight (60));
    
    knobRow.removeFromLeft (knobGap);
    speedKnob.setBounds (knobRow.removeFromLeft (knobWidth));
    knobRow.removeFromLeft (knobGap);
    rangeKnob.setBounds (knobRow.removeFromLeft (knobWidth));
    knobRow.removeFromLeft (knobGap);

    // Anchor knob with Auto toggle overlay
    auto anchorArea = knobRow;
    anchorKnob.setBounds (anchorArea);
    anchorAutoToggle.setBounds (anchorArea.removeFromTop (16).removeFromRight (48));

    // ── Expert section ──
    if (expertVisible)
    {
        bounds.removeFromTop (4);
        expertSection.setBounds (bounds);

        auto expertBounds = expertSection.getLocalBounds();
        const auto expertCount = 5;
        const auto expertGap = 6;
        const auto expertWidth = (expertBounds.getWidth() - (expertCount - 1) * expertGap) / expertCount;

        auto expertRow = expertBounds.removeFromTop (72);
        boostCeilingKnob.setBounds (expertRow.removeFromLeft (expertWidth));
        expertRow.removeFromLeft (expertGap);
        duckFloorKnob.setBounds (expertRow.removeFromLeft (expertWidth));
        expertRow.removeFromLeft (expertGap);
        noiseGateKnob.setBounds (expertRow.removeFromLeft (expertWidth));
        expertRow.removeFromLeft (expertGap);
        lookAheadKnob.setBounds (expertRow.removeFromLeft (expertWidth));
        expertRow.removeFromLeft (expertGap);
        presenceReleaseKnob.setBounds (expertRow);
    }

    expertSection.setVisible (expertVisible);

    helpOverlay.setBounds (getLocalBounds());
    aboutOverlay.setBounds (getLocalBounds());
}

void LiveFlowAudioProcessorEditor::updateAllTexts()
{
    const auto lang = isChinese ? i18n::Language::Chinese : i18n::Language::English;

    langButton.setButtonText (i18n::getText ("Btn_ToggleLang", lang));
    expertButton.setButtonText (i18n::getText ("Btn_Expert", lang));
    helpButton.setButtonText (i18n::getText ("Btn_Help", lang));
    aboutButton.setButtonText (i18n::getText ("Btn_About", lang));
    
    balanceKnob.setLabel (i18n::getText ("Label_Balance", lang));
    dynamicsKnob.setLabel (i18n::getText ("Label_Dynamics", lang));
    speedKnob.setLabel (i18n::getText ("Label_Speed", lang));
    rangeKnob.setLabel (i18n::getText ("Label_Range", lang));
    anchorKnob.setLabel (i18n::getText ("Label_Anchor", lang));
    
    anchorAutoToggle.setButtonText (i18n::getText ("Toggle_Auto", lang));
    expansionModeToggle.setButtonText (i18n::getText (expansionModeToggle.getToggleState() ? "Toggle_Expand" : "Toggle_Compress", lang));
    
    boostCeilingKnob.setLabel (i18n::getText ("Label_BoostCeiling", lang));
    duckFloorKnob.setLabel (i18n::getText ("Label_DuckFloor", lang));
    noiseGateKnob.setLabel (i18n::getText ("Label_NoiseGate", lang));
    lookAheadKnob.setLabel (i18n::getText ("Label_LookAhead", lang));
    presenceReleaseKnob.setLabel (i18n::getText ("Label_ReleaseHyst", lang));
    
    helpOverlay.updateLanguage (isChinese);
    aboutOverlay.updateLanguage (isChinese);
    repaint();
}

// ═══════════════════════════════════════════
// Formatters
// ═══════════════════════════════════════════

juce::String LiveFlowAudioProcessorEditor::formatSignedDb (const double value)
{
    const auto prefix = value > 0.05 ? "+" : "";
    return prefix + juce::String (value, 1) + " dB";
}

juce::String LiveFlowAudioProcessorEditor::formatPercent (const double value)
{
    return juce::String (value, 0) + "%";
}

juce::String LiveFlowAudioProcessorEditor::formatMilliseconds (const double value)
{
    return juce::String (value, 0) + " ms";
}

juce::String LiveFlowAudioProcessorEditor::formatDb (const double value)
{
    return juce::String (value, 1) + " dB";
}

std::unique_ptr<LiveFlowAudioProcessorEditor::SliderAttachment>
LiveFlowAudioProcessorEditor::attachSlider (const juce::String& parameterId, juce::Slider& slider)
{
    return std::make_unique<SliderAttachment> (processor.getValueTreeState(), parameterId, slider);
}
} // namespace liveflow
