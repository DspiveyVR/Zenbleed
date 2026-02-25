#include "PluginEditor.h"

#include "ZenbleedLookAndFeel.h"

PluginEditor::PluginEditor(PluginProcessor& p) : AudioProcessorEditor(&p), processorRef(p) {
    setLookAndFeel(&lookAndFeel);
#if DEBUG
    addAndMakeVisible(inspectButton);

    // this chunk of code instantiates and opens the melatonin inspector
    inspectButton.onClick = [&] {
        if (!inspector) {
            inspector = std::make_unique<melatonin::Inspector>(*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible(true);
    };
#endif

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    // NOTE: The size must be set, *before* we addAndMakeVisible.
    setSize(640, 480);

    titleLabel.setText("Zenbleed", juce::dontSendNotification);
    theLambel.setText("Computer running hot yet?", juce::dontSendNotification);

    // Gather a list of components to display.
    std::vector<juce::Component*> components = { &titleLabel,      &theLambel,        &openButton,     &lowSpeedButton,
                                                 &midSpeedButton,  &highSpeedButton,  &speedSlider,    &midiToggle,
                                                 &tunedToggle,     &noteLengthSlider, &velocitySlider, &keytrackToggle,
                                                 &fixedNoteSlider, &pitchBendSlider,  &eTETMode,       &rootNoteSlider,
                                                 &numeratorSlider, &denominatorSlider };
    // Display them
    for (auto* comp: components) {
        addAndMakeVisible(comp);
    }

    // Remove textboxes from sliders
    noteLengthSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    velocitySlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    fixedNoteSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    pitchBendSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    rootNoteSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    numeratorSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    denominatorSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);

    // speedKnob.setSliderStyle(juce::Slider::Rotary);
    speedSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    speedSlider.onValueChange = [this] {
        if (currentSpeedParam) {
            // Convert knob value to normalized (0-1)
            float normalized = currentSpeedParam->convertTo0to1((float)speedSlider.getValue());
            currentSpeedParam->setValueNotifyingHost(normalized);
        }
    };
    speedSlider.setValue(5.00);

    openButton.onClick = [this] { openButtonClicked(); };
    // FIXME: Hard coding parameter names is bad.
    lowSpeedButton.onClick = [this] { speedSliderButtonClicked("lowSpeed"); };
    midSpeedButton.onClick = [this] { speedSliderButtonClicked("midSpeed"); };
    highSpeedButton.onClick = [this] { speedSliderButtonClicked("highSpeed"); };
}

PluginEditor::~PluginEditor() { setLookAndFeel(nullptr); }

void PluginEditor::openButtonClicked() {
    chooser = std::make_unique<juce::FileChooser>("Select a sample... ", juce::File {}, "*.wav");
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file != juce::File {}) {
            getProcessor().sampleOscillator.get()->loadSampleFromFile(file);
        }
    });
}

void PluginEditor::speedSliderButtonClicked(const juce::String& parameter) {
    if (parameter == "lowSpeed") {
        currentSpeedParam = processorRef.getParametersApvts().getParameter("lowSpeed");
        speedSlider.setRange(0.0, 16.0);
    } else if (parameter == "midSpeed") {
        currentSpeedParam = processorRef.getParametersApvts().getParameter("midSpeed");
        speedSlider.setRange(0.0, 64.0);
    } else if (parameter == "highSpeed") {
        currentSpeedParam = processorRef.getParametersApvts().getParameter("highSpeed");
        speedSlider.setRange(0.0, 256.0);
    }

    if (currentSpeedParam) {
        // Move knob to match parameter value
        float normalized = currentSpeedParam->getValue(); // normalized 0-1
        float realValue = currentSpeedParam->convertFrom0to1(normalized);
        speedSlider.setValue(realValue, juce::dontSendNotification);
    }
}

PluginProcessor& PluginEditor::getProcessor() const { return static_cast<PluginProcessor&>(processor); }

void PluginEditor::paint(juce::Graphics& g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    // auto area = getLocalBounds();
    // g.setColour(juce::Colours::white);
    // g.setFont(16.0f);
    // auto helloWorld = juce::String("Hello from ") + PRODUCT_NAME_WITHOUT_VERSION + " v" VERSION + " running in " + CMAKE_BUILD_TYPE;
    // g.drawText(helloWorld, area.removeFromTop(150), juce::Justification::centred, false);
    juce::AudioProcessorEditor::paint(g);
}

void PluginEditor::resized() {
    auto area = getLocalBounds(); // A

    // Split `area` into proper regions.
    auto titleArea = area.removeFromTop(50); // B
    auto leftArea = area.removeFromLeft(area.getWidth() / 2); // C
    auto rightArea = area; // G

    auto fileArea = leftArea.removeFromBottom(70); // F
    auto featureArea = leftArea.removeFromTop(leftArea.getHeight() / 2); // D
    auto lowerArea = leftArea; // E

    auto lambelArea = rightArea.removeFromBottom(70); // N
    auto radioArea = rightArea.removeFromTop(2 * rightArea.getHeight() / 3); // H
    auto buttonRow = rightArea.removeFromTop(rightArea.getHeight() / 2); // I
    auto sliderArea = rightArea; // M

    // BUG: There is an issue that causes the sliders to be of varying sizes.
    //      I will need to find a fix for this. ~ Ilia

    // Title Area
    titleLabel.setBounds(titleArea.reduced(10).removeFromRight(100));

    // File Area
    {
        fileArea = fileArea.reduced(10);
        openButton.setBounds(fileArea.removeFromLeft(100));
        openButton.setSize(100, 50);
        // TODO: Add label with filename.
    }

    // Feature Area
    {
        featureArea = featureArea.reduced(10);
        // TODO: Make midi / sampler toggle more intuitive.
        midiToggle.setBounds(featureArea.removeFromTop(50));
        noteLengthSlider.setBounds(featureArea.removeFromTop(50));
    }

    // Lower Area
    {
        lowerArea = lowerArea.reduced(10);
        velocitySlider.setBounds(lowerArea.removeFromTop(50));
        keytrackToggle.setBounds(lowerArea.removeFromTop(50));
        fixedNoteSlider.setBounds(lowerArea.removeFromTop(50));
        pitchBendSlider.setBounds(lowerArea.removeFromTop(50));
    }

    // Lambel Area
    {
        lambelArea = lambelArea.reduced(10);
        theLambel.setSize(lambelArea.getWidth() / 2, lambelArea.getHeight());
        theLambel.setBoundsToFit(lambelArea, juce::Justification::centred, true);
    }

    // Radio Area
    {
        radioArea = radioArea.reduced(10);
        // TODO: Convert to three way radio.
        eTETMode.setBounds(radioArea.removeFromTop(50));
        tunedToggle.setBounds(radioArea.removeFromTop(50));
        rootNoteSlider.setBounds(radioArea.removeFromTop(50));
        numeratorSlider.setBounds(radioArea.removeFromTop(50));
        denominatorSlider.setBounds(radioArea.removeFromTop(50));
    }

    // Button Row
    {
        buttonRow = buttonRow.reduced(10);
        auto lowButtonArea = buttonRow.removeFromLeft(buttonRow.getWidth() / 3); // J
        auto midButtonArea = buttonRow.removeFromLeft(buttonRow.getWidth() / 2); // K
        auto highButtonArea = buttonRow; // L
        lowSpeedButton.setBounds(lowButtonArea);
        midSpeedButton.setBounds(midButtonArea);
        highSpeedButton.setBounds(highButtonArea);
    }

    // Slider Area
    {
        sliderArea = sliderArea.reduced(10);
        speedSlider.setBounds(sliderArea);
    }

    // Inspect Button
    inspectButton.setSize(100, 50);
    inspectButton.setAlwaysOnTop(true);
    inspectButton.setBoundsToFit(getScreenBounds(), juce::Justification::bottomRight, true);
}
