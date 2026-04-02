#include "LiveFlowAudioProcessorEditor.h"

#include "LiveFlowAudioProcessor.h"

namespace
{
constexpr int defaultEditorWidth = 1260;
constexpr int defaultEditorHeight = 620;
constexpr int profilerPanelWidth = 340;

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

juce::Colour profilerAccent (const int index)
{
    static const std::array colours {
        juce::Colour (0xffff6b9d), // Max Record — pink
        juce::Colour (0xffb088f9), // Zones — purple
        juce::Colour (0xff6dd5fa)  // Bands — sky blue
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
                            [] (double v) { return formatMilliseconds (v); }),
      maxRecordKnob ("Max Record", profilerAccent (0),
                      [] (double v) { return juce::String (static_cast<int> (v)) + " min"; }),
      numZonesKnob ("Zones", profilerAccent (1),
                     [] (double v) { return juce::String (static_cast<int> (v)); }),
      numBandsKnob ("EQ Bands", profilerAccent (2),
                     [] (double v) { return juce::String (static_cast<int> (v)); })
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);
    setResizable (true, true);
    setResizeLimits (1060, 540, 1600, 900);
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
        const auto lang = isChinese ? i18n::Language::Chinese : i18n::Language::English;
        juce::AlertWindow::showMessageBoxAsync (
            juce::AlertWindow::InfoIcon,
            i18n::getText ("About_Title", lang),
            i18n::getText ("About_Desc", lang) + "\n\nVersion: " + juce::String(LIVEFLOW_VERSION_EXT));
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

    // ── Smart Track Profiler Panel ──
    // Listen button (red record style)
    listenButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x28ff6b9d));
    listenButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffff6b9d));
    listenButton.onClick = [this]
    {
        auto& prof = processor.getProfiler();
        const auto state = prof.getState();

        if (state == ProfilerState::Idle || state == ProfilerState::Done)
        {
            processor.startProfiling();
        }
        else if (state == ProfilerState::Recording)
        {
            // Generate a default name
            const auto name = "Profile " + juce::String (static_cast<int> (processor.getProfiles().size()) + 1);
            processor.stopProfiling (name);
        }
    };
    addAndMakeVisible (listenButton);

    // Profile selector
    profileSelector.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff14202e));
    profileSelector.setColour (juce::ComboBox::textColourId, juce::Colour (0xffeef4fb));
    profileSelector.setColour (juce::ComboBox::outlineColourId, juce::Colour (0x30ffffff));
    profileSelector.onChange = [this]
    {
        const auto selected = profileSelector.getSelectedId();
        processor.setActiveProfileIndex (selected - 1);
    };
    addAndMakeVisible (profileSelector);
    updateProfileSelector();

    // Export / Import / Delete buttons
    for (auto* btn : { &exportButton, &importButton, &deleteButton })
    {
        btn->setColour (juce::TextButton::buttonColourId, juce::Colour (0x10ffffff));
        btn->setColour (juce::TextButton::textColourOffId, juce::Colour (0xff9fb0c7));
        addAndMakeVisible (*btn);
    }

    exportButton.onClick = [this]
    {
        const auto idx = processor.getActiveProfileIndex();
        if (idx < 0) return;

        auto chooser = std::make_shared<juce::FileChooser> ("Export Profile", juce::File {}, "*.json");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode, [this, idx, chooser] (const juce::FileChooser& fc)
        {
            if (auto result = fc.getResult(); result != juce::File {})
                processor.exportProfileToJSON (idx, result.withFileExtension ("json"));
        });
    };

    importButton.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser> ("Import Profile", juce::File {}, "*.json");
        chooser->launchAsync (juce::FileBrowserComponent::openMode, [this, chooser] (const juce::FileChooser& fc)
        {
            if (auto result = fc.getResult(); result.existsAsFile())
            {
                processor.importProfileFromJSON (result);
                updateProfileSelector();
            }
        });
    };

    deleteButton.onClick = [this]
    {
        const auto idx = processor.getActiveProfileIndex();
        if (idx >= 0)
        {
            processor.deleteProfile (idx);
            updateProfileSelector();
        }
    };

    // Status & recording time labels
    profilerStatusLabel.setColour (juce::Label::textColourId, juce::Colour (0xff9fb0c7));
    profilerStatusLabel.setFont (juce::FontOptions (11.0f));
    addAndMakeVisible (profilerStatusLabel);

    recordingTimeLabel.setColour (juce::Label::textColourId, juce::Colour (0xffff6b9d));
    recordingTimeLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (recordingTimeLabel);

    // Profiler settings knobs
    maxRecordKnob.setLabel ("Max Record");
    numZonesKnob.setLabel ("Zones");
    numBandsKnob.setLabel ("EQ Bands");
    addAndMakeVisible (maxRecordKnob);
    addAndMakeVisible (numZonesKnob);
    addAndMakeVisible (numBandsKnob);

    maxRecordAttachment = attachSlider (param::profileMaxMinutes, maxRecordKnob.getSlider());
    numZonesAttachment = attachSlider (param::profileNumZones, numZonesKnob.getSlider());
    numBandsAttachment = attachSlider (param::profileNumBands, numBandsKnob.getSlider());

    updateAllTexts();

    startTimerHz (30);
    setSize (defaultEditorWidth, defaultEditorHeight);
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

    // Update profiler UI
    updateProfilerUI();

    // Auto-pick up newly generated profiles
    const auto& prof = processor.getProfiler();
    if (prof.getState() == ProfilerState::Done)
    {
        // Transfer profile from profiler to processor's list
        auto& profiles = const_cast<std::vector<TrackProfile>&> (processor.getProfiles());
        profiles.push_back (prof.getGeneratedProfile());
        const_cast<TrackProfiler&> (prof).acknowledgeProfile();
        processor.setActiveProfileIndex (static_cast<int> (profiles.size()) - 1);
        updateProfileSelector();
    }
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
    graphics.drawText (juce::String(LIVEFLOW_VERSION_EXT), titleArea.removeFromLeft (85), juce::Justification::centredLeft);
    graphics.setColour (juce::Colour (0xff9fb0c7));
    graphics.setFont (juce::FontOptions (9.5f));
    const auto lang = isChinese ? i18n::Language::Chinese : i18n::Language::English;
    graphics.drawText (i18n::getText ("Header_Sub", lang), titleArea, juce::Justification::centredLeft);

    // ── Profiler Panel Background ──
    if (profilerPanelVisible)
    {
        auto panelBounds = getLocalBounds();
        panelBounds.removeFromLeft (panelBounds.getWidth() - profilerPanelWidth);

        // Panel background
        graphics.setColour (juce::Colour (0xff0d1825));
        graphics.fillRect (panelBounds.toFloat());

        // Left edge separator
        graphics.setColour (juce::Colour (0x20ffffff));
        graphics.drawVerticalLine (panelBounds.getX(), 0.0f, static_cast<float> (panelBounds.getHeight()));

        // Panel title
        auto panelTitle = panelBounds.reduced (16, 0).removeFromTop (40);
        graphics.setColour (juce::Colour (0xffff6b9d));
        graphics.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        graphics.drawText (i18n::getText ("Panel_Profiler", lang),
                           panelTitle, juce::Justification::centredLeft);

        // Record progress bar background
        const auto& prof = processor.getProfiler();
        if (prof.getState() == ProfilerState::Recording)
        {
            auto progressArea = panelBounds.reduced (16, 0);
            progressArea.removeFromTop (110);
            auto barBounds = progressArea.removeFromTop (6).toFloat();

            // Background
            graphics.setColour (juce::Colour (0x20ffffff));
            graphics.fillRoundedRectangle (barBounds, 3.0f);

            // Fill
            auto fillBounds = barBounds.withWidth (barBounds.getWidth() * prof.getProgress());
            graphics.setColour (juce::Colour (0xffff6b9d));
            graphics.fillRoundedRectangle (fillBounds, 3.0f);
        }
    }
}

void LiveFlowAudioProcessorEditor::resized()
{
    const auto leftPanelWidth = profilerPanelVisible
                                    ? getWidth() - profilerPanelWidth
                                    : getWidth();
    auto bounds = getLocalBounds().withWidth (leftPanelWidth).reduced (14);
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

    // ── Smart Track Profiler Right Panel ──
    if (profilerPanelVisible)
    {
        auto panelBounds = getLocalBounds();
        panelBounds.removeFromLeft (panelBounds.getWidth() - profilerPanelWidth);
        panelBounds = panelBounds.reduced (16, 0);
        panelBounds.removeFromTop (44); // Panel title space

        // Listen button
        listenButton.setBounds (panelBounds.removeFromTop (36));
        panelBounds.removeFromTop (8);

        // Recording time label
        recordingTimeLabel.setBounds (panelBounds.removeFromTop (18));
        panelBounds.removeFromTop (2);

        // Progress bar space (painted in paint())
        panelBounds.removeFromTop (8);

        // Status label
        profilerStatusLabel.setBounds (panelBounds.removeFromTop (16));
        panelBounds.removeFromTop (12);

        // Profile selector
        profileSelector.setBounds (panelBounds.removeFromTop (28));
        panelBounds.removeFromTop (8);

        // Export / Import / Delete row
        auto btnRow = panelBounds.removeFromTop (26);
        const auto mgmtBtnW = (btnRow.getWidth() - 8) / 3;
        exportButton.setBounds (btnRow.removeFromLeft (mgmtBtnW));
        btnRow.removeFromLeft (4);
        importButton.setBounds (btnRow.removeFromLeft (mgmtBtnW));
        btnRow.removeFromLeft (4);
        deleteButton.setBounds (btnRow);
        panelBounds.removeFromTop (16);

        // Profiler settings knobs
        auto settingsRow = panelBounds.removeFromTop (72);
        const auto settingsKnobW = (settingsRow.getWidth() - 8) / 3;
        maxRecordKnob.setBounds (settingsRow.removeFromLeft (settingsKnobW));
        settingsRow.removeFromLeft (4);
        numZonesKnob.setBounds (settingsRow.removeFromLeft (settingsKnobW));
        settingsRow.removeFromLeft (4);
        numBandsKnob.setBounds (settingsRow);
    }

    helpOverlay.setBounds (getLocalBounds());
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

    // Profiler panel texts
    maxRecordKnob.setLabel (i18n::getText ("Label_MaxRecord", lang));
    numZonesKnob.setLabel (i18n::getText ("Label_NumZones", lang));
    numBandsKnob.setLabel (i18n::getText ("Label_NumBands", lang));
    exportButton.setButtonText (i18n::getText ("Btn_Export", lang));
    importButton.setButtonText (i18n::getText ("Btn_Import", lang));
    deleteButton.setButtonText (i18n::getText ("Btn_Delete", lang));

    updateProfilerUI();
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

// ═══════════════════════════════════════════
// Profiler Helpers
// ═══════════════════════════════════════════

void LiveFlowAudioProcessorEditor::updateProfileSelector()
{
    profileSelector.clear (juce::dontSendNotification);
    const auto lang = isChinese ? i18n::Language::Chinese : i18n::Language::English;

    profileSelector.addItem (i18n::getText ("Profile_None", lang), 1);

    const auto& profiles = processor.getProfiles();

    for (int i = 0; i < static_cast<int> (profiles.size()); ++i)
        profileSelector.addItem (profiles[static_cast<size_t> (i)].name, i + 2);

    const auto activeIdx = processor.getActiveProfileIndex();
    profileSelector.setSelectedId (activeIdx >= 0 ? activeIdx + 2 : 1, juce::dontSendNotification);
}

void LiveFlowAudioProcessorEditor::updateProfilerUI()
{
    const auto lang = isChinese ? i18n::Language::Chinese : i18n::Language::English;
    const auto& prof = processor.getProfiler();
    const auto state = prof.getState();

    // Update Listen button text and color
    switch (state)
    {
        case ProfilerState::Recording:
        {
            listenButton.setButtonText (i18n::getText ("Btn_StopListen", lang));
            listenButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x40ff3333));
            listenButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffff4444));

            // Update recording time display
            const auto secs = static_cast<int> (prof.getRecordedSeconds());
            const auto maxSecs = static_cast<int> (prof.getMaxSeconds());
            recordingTimeLabel.setText (
                juce::String::formatted ("%d:%02d / %d:%02d",
                                        secs / 60, secs % 60,
                                        maxSecs / 60, maxSecs % 60),
                juce::dontSendNotification);
            break;
        }
        case ProfilerState::Analyzing:
        {
            listenButton.setButtonText (i18n::getText ("Profile_Analyzing", lang));
            listenButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x28f7d57a));
            listenButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xfff7d57a));
            recordingTimeLabel.setText ("", juce::dontSendNotification);
            break;
        }
        default:
        {
            listenButton.setButtonText (i18n::getText ("Btn_Listen", lang));
            listenButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x28ff6b9d));
            listenButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffff6b9d));
            recordingTimeLabel.setText ("", juce::dontSendNotification);
            break;
        }
    }

    // Status text
    juce::String statusText;
    const auto activeIdx = processor.getActiveProfileIndex();

    if (state == ProfilerState::Recording)
        statusText = i18n::getText ("Profile_Recording", lang);
    else if (state == ProfilerState::Analyzing)
        statusText = i18n::getText ("Profile_Analyzing", lang);
    else if (state == ProfilerState::Error)
        statusText = i18n::getText ("Profile_Error", lang);
    else if (activeIdx >= 0)
        statusText = i18n::getText ("Profile_Ready", lang);

    profilerStatusLabel.setText (statusText, juce::dontSendNotification);

    // Trigger repaint for progress bar
    if (state == ProfilerState::Recording)
        repaint();
}

} // namespace liveflow
