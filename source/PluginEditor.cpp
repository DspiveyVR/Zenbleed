#include "PluginEditor.h"
#include "../build/juce_binarydata_Assets/JuceLibraryCode/BinaryData.h"

PluginEditor::PluginEditor(PluginProcessor& p) : AudioProcessorEditor(&p), processorRef(p) {
    setLookAndFeel(&lookAndFeel);

#if DEBUG
    addAndMakeVisible(inspectButton);
    inspectButton.onClick = [&] {
        if (!inspector) {
            inspector = std::make_unique<melatonin::Inspector>(*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }
        inspector->setVisible(true);
    };
#endif

    startTimerHz(30);

    // --- Window Setup ---
    setResizable(true, true);
    constrainer.setFixedAspectRatio(originalWidth / originalHeight);
    constrainer.setMinimumWidth(525);
    constrainer.setMinimumHeight(390);
    setConstrainer(&constrainer);
    setSize(originalWidth, originalHeight);

    auto& apvts = processorRef.getParametersApvts();

    // --- 1. Speed Range Parameter Sync ---
    // Listen for changes to speedRange (for automation/presets)
    apvts.addParameterListener("speedRange", this);

    auto setupSpeedButton = [this](juce::TextButton& b) {
        b.setClickingTogglesState(false);
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colours::lightgrey);
        b.setColour(juce::TextButton::textColourOnId, juce::Colours::red);
        addAndMakeVisible(b);
    };

    setupSpeedButton(lowSpeedButton);
    setupSpeedButton(midSpeedButton);
    setupSpeedButton(highSpeedButton);

    // Initial State Sync for Speed Range
    int initialRange = (int)*apvts.getRawParameterValue("speedRange");
    lowSpeedButton.setToggleState(initialRange == 0, juce::dontSendNotification);
    midSpeedButton.setToggleState(initialRange == 1, juce::dontSendNotification);
    highSpeedButton.setToggleState(initialRange == 2, juce::dontSendNotification);

    // Sync the slider attachment to the initial range
    juce::String initialID = (initialRange == 0) ? "lowSpeed" : (initialRange == 1) ? "midSpeed" : "highSpeed";
    speedSliderButtonClicked(initialID);

    // Button Callbacks: Update the internal choice parameter
    lowSpeedButton.onClick = [this, &apvts] { apvts.getParameter("speedRange")->setValueNotifyingHost(0.0f); };
    midSpeedButton.onClick = [this, &apvts] { apvts.getParameter("speedRange")->setValueNotifyingHost(0.5f); };
    highSpeedButton.onClick = [this, &apvts] { apvts.getParameter("speedRange")->setValueNotifyingHost(1.0f); };

    logo = juce::ImageCache::getFromMemory(BinaryData::logo_png, BinaryData::logo_pngSize);
    background = juce::ImageCache::getFromMemory(BinaryData::background_jpg, BinaryData::background_jpgSize);
    logoComponent.setImage(logo, juce::RectanglePlacement::centred);
    addAndMakeVisible(logoComponent);

    sampleNameLabel.setText("No sample loaded", juce::dontSendNotification);
    sampleNameLabel.setJustificationType(juce::Justification::centred);
    sampleNameLabel.setFont(juce::Font(13.0f, juce::Font::italic));
    sampleNameLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    if (apvts.state.hasProperty("SamplePath")) {
        juce::File sampleFile(apvts.state.getProperty("SamplePath").toString());
        sampleNameLabel.setText(sampleFile.getFileName(), juce::dontSendNotification);
        sampleNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    }
    addAndMakeVisible(sampleNameLabel);

    // --- 3. Attachments ---
    midiToggleAttachment = std::make_unique<ButtonAttachment>(apvts, "isMidiMode", midiToggle);
    keytrackAttachment = std::make_unique<ButtonAttachment>(apvts, "isKeytrack", keytrackToggle);

    noteLengthAttachment = std::make_unique<SliderAttachment>(apvts, "noteLength", noteLengthSlider);
    velocityAttachment = std::make_unique<SliderAttachment>(apvts, "velocity", velocitySlider);
    fixedNoteAttachment = std::make_unique<SliderAttachment>(apvts, "fixedNoteNumber", fixedNoteSlider);
    pitchBendAttachment = std::make_unique<SliderAttachment>(apvts, "samplePitchBendRatio", pitchBendSlider);

    // Operation Mode Dropdown
    modeLabel.setText("Operation Mode", juce::dontSendNotification);
    addAndMakeVisible(modeLabel);
    modeSelector.addItem("Default", 1);
    modeSelector.addItem("ETET", 2);
    modeSelector.addItem("Tuned", 3);
    addAndMakeVisible(modeSelector);
    modeAttachment = std::make_unique<ComboBoxAttachment>(apvts, "operationMode", modeSelector);

    rootNoteAttachment = std::make_unique<SliderAttachment>(apvts, "etetRootNote", rootNoteSlider);
    numeratorAttachment = std::make_unique<SliderAttachment>(apvts, "etetNumerator", numeratorSlider);
    denominatorAttachment = std::make_unique<SliderAttachment>(apvts, "etetDenominator", denominatorSlider);

    // --- 4. Slider Setup Helper ---
    auto setupSlider = [this](
                           juce::Slider& s,
                           juce::Label& vLabel,
                           juce::Label& nLabel,
                           juce::String name,
                           juce::String suffix,
                           bool isNoteLabel = false
                       ) {
        nLabel.setText(name, juce::dontSendNotification);
        nLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(nLabel);

        vLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(vLabel);

        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        s.onValueChange = [this, &s, &vLabel, suffix, isNoteLabel] {
            isNoteLabel ? updateNoteLabelText(vLabel, s) : updateLabelText(vLabel, s, suffix);
        };
        addAndMakeVisible(s);

        isNoteLabel ? updateNoteLabelText(vLabel, s) : updateLabelText(vLabel, s, suffix);
    };

    setupSlider(noteLengthSlider, noteLengthLabel, noteLengthName, "Length", "");
    setupSlider(velocitySlider, velocityLabel, velocityName, "Velocity", "");
    setupSlider(fixedNoteSlider, fixedNoteLabel, fixedNoteName, "Note", "", true);
    setupSlider(pitchBendSlider, pitchBendLabel, pitchBendName, "Pitch", "st");
    setupSlider(rootNoteSlider, rootNoteLabel, rootNoteName, "Root", "", true);
    setupSlider(numeratorSlider, numeratorLabel, numeratorName, "Num", "");
    setupSlider(denominatorSlider, denominatorLabel, denominatorName, "Denom", "");

    speedName.setText("Speed Range", juce::dontSendNotification);
    speedName.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(speedName);

    speedSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    speedSlider.onValueChange = [this] { updateLabelText(speedValueLabel, speedSlider, "x"); };
    addAndMakeVisible(speedSlider);
    addAndMakeVisible(speedValueLabel);

    addAndMakeVisible(titleLabel);

    theLambel.setReadOnly(true);
    theLambel.setMultiLine(true);
    theLambel.setReadOnly(true);
    theLambel.setCaretVisible(false);
    theLambel.setScrollbarsShown(false);
    theLambel.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    theLambel.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);

    theLambel.setFont(juce::FontOptions(40.0f));
    addAndMakeVisible(theLambel);
    addAndMakeVisible(openButton);
    addAndMakeVisible(midiToggle);
    addAndMakeVisible(keytrackToggle);
    bpmSpeedometerLabel.setFont(juce::FontOptions(30.0f));
    addAndMakeVisible(bpmSpeedometerLabel);
    hzSpeedometerLabel.setFont(juce::FontOptions(30.0f));
    addAndMakeVisible(hzSpeedometerLabel);
    bpmTextLabel.setFont(juce::FontOptions(30.0f));
    bpmTextLabel.setText("BPM", juce::dontSendNotification);
    addAndMakeVisible(bpmTextLabel);
    hzTextLabel.setFont(juce::FontOptions(30.0f));
    hzTextLabel.setText("HZ", juce::dontSendNotification);
    addAndMakeVisible(hzTextLabel);

    openButton.onClick = [this] { openButtonClicked(); };
}

PluginEditor::~PluginEditor() {
    processorRef.getParametersApvts().removeParameterListener("speedRange", this);
    setLookAndFeel(nullptr);
}

// For updating decorative UI elements
void PluginEditor::timerCallback() {
    float bpmSpeedometer = processorRef.bpmSpeedometer.load();

    bpmSpeedometerLabel.setText(juce::String(bpmSpeedometer), juce::dontSendNotification);
    hzSpeedometerLabel.setText(juce::String(bpmSpeedometer / 60), juce::dontSendNotification);
    setLambel((long)bpmSpeedometer);
}

void PluginEditor::setLambel(long bpm) {
    // 1. Lower Bound Check: If bpm is lower than the first range's start
    if (bpm < lambels.front().speedRange[0]) {
        auto lambel = lambels.front();
        theLambel.clear();
        theLambel.setText(lambel.text, juce::dontSendNotification);
        theLambel.setColour(juce::TextEditor::textColourId, lambel.color);
        theLambel.applyFontToAllText(juce::FontOptions(40.0f));
        return;
    }

    // 2. Range Check: Iterate through to find a match
    for (const auto& lambel: lambels) {
        if (bpm >= lambel.speedRange[0] && bpm <= lambel.speedRange[1]) {
            theLambel.clear();
            theLambel.setText(lambel.text, juce::dontSendNotification);
            theLambel.setColour(juce::TextEditor::textColourId, lambel.color);
            theLambel.applyFontToAllText(juce::FontOptions(40.0f));
            return;
        }
    }

    // 3. Upper Bound Check: If we didn't find a match and bpm is higher
    // (This handles bpm > 1001 or any gaps in your ranges)
    auto lambel = lambels.back();
    theLambel.clear();
    theLambel.setText(lambel.text, juce::dontSendNotification);
    theLambel.setColour(juce::TextEditor::textColourId, lambel.color);
    theLambel.applyFontToAllText(juce::FontOptions(40.0f));
    return;
}

// Sync UI when parameter changes via automation or dropdown
void PluginEditor::parameterChanged(const juce::String& parameterID, float newValue) {
    if (parameterID == "speedRange") {
        juce::MessageManager::callAsync([this, newValue] {
            int index = juce::roundToInt(newValue);
            lowSpeedButton.setToggleState(index == 0, juce::dontSendNotification);
            midSpeedButton.setToggleState(index == 1, juce::dontSendNotification);
            highSpeedButton.setToggleState(index == 2, juce::dontSendNotification);

            juce::String id = (index == 0) ? "lowSpeed" : (index == 1) ? "midSpeed" : "highSpeed";
            speedSliderButtonClicked(id);
        });
    }
}

void PluginEditor::updateLabelText(juce::Label& label, juce::Slider& slider, juce::String suffix) {
    label.setText(juce::String(slider.getValue(), 2) + suffix, juce::dontSendNotification);
}

void PluginEditor::updateNoteLabelText(juce::Label& label, juce::Slider& slider) {
    label.setText(
        juce::MidiMessage::getMidiNoteName((int)slider.getValue(), true, true, 4), juce::dontSendNotification
    );
}

void PluginEditor::openButtonClicked() {
    chooser = std::make_unique<juce::FileChooser>("Select a sample...", juce::File {}, "*.wav");
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.existsAsFile()) {
            getProcessor().sampleOscillator.get()->loadSampleFromFile(file);
            sampleNameLabel.setText(file.getFileName(), juce::dontSendNotification);
            sampleNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        }
    });
}

void PluginEditor::speedSliderButtonClicked(const juce::String& parameterID) {
    speedAttachment = std::make_unique<SliderAttachment>(processorRef.getParametersApvts(), parameterID, speedSlider);
    updateLabelText(speedValueLabel, speedSlider);
}

PluginProcessor& PluginEditor::getProcessor() const { return static_cast<PluginProcessor&>(processor); }

void PluginEditor::paint(juce::Graphics& g) {
    // 1. Fill with black first to prevent "ghosting" artifacts
    g.fillAll(juce::Colours::black);

    // 2. Draw the background image
    if (background.isValid()) {
        // We use originalWidth and originalHeight because your resized()
        // scale transform handles the window stretching for us.
        // stretchToFit ensures the image maps perfectly to your UI layout.
        g.drawImageWithin(background, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::stretchToFit, false);
    }

    // 3. Optional: Darken the image by 20% to make the red sliders pop
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.fillRect(getLocalBounds());
}

void PluginEditor::resized() {
    // 1. Calculate how much we've scaled from the original design
    float scale = getWidth() / originalWidth;
    // 2. Apply the transform to the Editor
    // This scales the DRAWING, but doesn't change the component's internal bounds
    setTransform(juce::AffineTransform::scale(scale));
    auto area = getLocalBounds().reduced(20);
    // Header with Logo
    auto header = area.removeFromTop(70);
    // 1. Define the width you want for the logo area
    int logoWidth = 180;
    // 2. Create a rectangle of that width, centered within the header
    auto imageRect = header.withSizeKeepingCentre(logoWidth, header.getHeight());
    imageRect.translate(22, -5);
    // 3. Apply your expansion
    imageRect.expand(100, 100);
    logoComponent.setBounds(imageRect);
    titleLabel.setBounds(header.removeFromLeft(120));
    auto mainColumns = area;
    auto leftCol = mainColumns.removeFromLeft(mainColumns.getWidth() / 2).reduced(10);
    auto rightCol = mainColumns.reduced(10);
    auto addFullRow = [](auto& area, auto& nameLabel, auto& slider, auto& valueLabel) {
        auto row = area.removeFromTop(35);
        nameLabel.setBounds(row.removeFromLeft(80));
        valueLabel.setBounds(row.removeFromRight(80));
        slider.setBounds(row);
    };
    // --- Left Column ---
    auto openArea = leftCol.removeFromTop(80);
    openButton.setBounds(openArea.removeFromTop(40).reduced(20, 0));
    sampleNameLabel.setBounds(openArea.removeFromTop(30));
    leftCol.removeFromTop(5);
    midiToggle.setBounds(leftCol.removeFromTop(30));
    addFullRow(leftCol, noteLengthName, noteLengthSlider, noteLengthLabel);
    addFullRow(leftCol, velocityName, velocitySlider, velocityLabel);
    keytrackToggle.setBounds(leftCol.removeFromTop(30));
    addFullRow(leftCol, fixedNoteName, fixedNoteSlider, fixedNoteLabel);
    addFullRow(leftCol, pitchBendName, pitchBendSlider, pitchBendLabel);

    const int constrainedWidth = 150;
    auto row = leftCol.removeFromTop(65).withSizeKeepingCentre(constrainedWidth, leftCol.removeFromTop(55).getHeight());
    row.translate(-30, 0);

    bpmSpeedometerLabel.setBounds(row);
    auto bpmTextRect = row;
    bpmTextRect.translate(constrainedWidth, 0);
    bpmTextLabel.setBounds(bpmTextRect);

    auto hzRect = row;
    hzRect.translate(0, 60);
    hzSpeedometerLabel.setBounds(hzRect);
    auto hzTextRect = hzRect;
    hzTextRect.translate(constrainedWidth, 0);
    hzTextLabel.setBounds(hzTextRect);

    // --- Right Column ---
    // Replace the old eTETMode and tunedToggle lines with this:
    auto modeArea = rightCol.removeFromTop(40);
    modeLabel.setBounds(modeArea.removeFromLeft(100));
    modeSelector.setBounds(modeArea);
    // --- Right Column ---
    addFullRow(rightCol, rootNoteName, rootNoteSlider, rootNoteLabel);
    addFullRow(rightCol, numeratorName, numeratorSlider, numeratorLabel);
    addFullRow(rightCol, denominatorName, denominatorSlider, denominatorLabel);
    rightCol.removeFromTop(15);
    speedName.setBounds(rightCol.removeFromTop(25));
    auto speedButtons = rightCol.removeFromTop(35);
    auto btnW = speedButtons.getWidth() / 3;
    lowSpeedButton.setBounds(speedButtons.removeFromLeft(btnW).reduced(2));
    midSpeedButton.setBounds(speedButtons.removeFromLeft(btnW).reduced(2));
    highSpeedButton.setBounds(speedButtons.reduced(2));
    speedSlider.setBounds(rightCol.removeFromTop(40));
    speedValueLabel.setBounds(rightCol.removeFromTop(20));

    const int constrainedWidthLambel = 350;
    auto lambelRect = rightCol.removeFromTop(60).withSizeKeepingCentre(constrainedWidthLambel, 150);
    lambelRect.translate(0, 60);
    theLambel.setBounds(lambelRect);

    inspectButton.setBounds(getWidth() - 110, getHeight() - 40, 100, 30);
}
