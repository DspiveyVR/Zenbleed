#include "PluginEditor.h"

#include "ZenbleedLookAndFeel.h"

PluginEditor::PluginEditor(PluginProcessor& p) : AudioProcessorEditor(&p), processorRef(p) {
    setLookAndFeel(&lookAndFeel);
#if DEBUG
    addAndMakeVisible(inspectButton);

    // this chunk of code instantiates and opens the melatonin inspector
    inspectButton.onClick = [&] {
        if(!inspector) {
            inspector = std::make_unique<melatonin::Inspector>(*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible(true);
    };
#endif

    const juce::StringArray speedRanges = processorRef.getSpeedRangeChoices();
    for (size_t i = 0; i < speedRanges.size(); i++) {
        speedRangeBox.addItem(speedRanges[i], i + 1);
    }

    speedRangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processorRef.getParametersApvts(), "speedRange", speedRangeBox);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    // NOTE: The size must be set, *before* we addAndMakeVisible.
    setSize(640, 480);

    addAndMakeVisible(openButton);
    addAndMakeVisible(speedRangeBox);

    addAndMakeVisible(lowSpeedButton);
    addAndMakeVisible(midSpeedButton);
    addAndMakeVisible(highSpeedButton);
    addAndMakeVisible(speedSlider);

    addAndMakeVisible(midiToggle);
    addAndMakeVisible(tunedToggle);
    addAndMakeVisible(titleLabel);

    titleLabel.setText("Lambel", juce::dontSendNotification);


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
    lowSpeedButton.onClick = [this] {speedSliderButtonClicked("lowSpeed");};
    midSpeedButton.onClick = [this] {speedSliderButtonClicked("midSpeed");};
    highSpeedButton.onClick = [this] {speedSliderButtonClicked("highSpeed");};

}

PluginEditor::~PluginEditor() {
    setLookAndFeel(nullptr);
}

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
    if (parameter == "lowSpeed")
    {
        currentSpeedParam = processorRef.getParametersApvts().getParameter("lowSpeed");
        speedSlider.setRange(0.0, 16.0);
    }
    else if (parameter == "midSpeed")
    {
        currentSpeedParam = processorRef.getParametersApvts().getParameter("midSpeed");
        speedSlider.setRange(0.0, 64.0);
    }
    else if (parameter == "highSpeed")
    {
        currentSpeedParam = processorRef.getParametersApvts().getParameter("highSpeed");
        speedSlider.setRange(0.0, 256.0);
    }

    if (currentSpeedParam)
    {
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
    // layout the positions of your child components here
    juce::AudioProcessorEditor::resized();
    // openButton.setBounds((getWidth() / 2) - (100 / 2), 150, 100, 40);
    // speedRangeBox.setBounds((getWidth() / 2) - (100 / 2), 200, 100, 40);

    // Speed controls
    speedSlider.setBounds(getScreenBounds().withSizeKeepingCentre(180,30)); // 180x180
    midSpeedButton.setBounds(speedSlider.getScreenX() + speedSlider.getWidth()/2 - 25, speedSlider.getScreenY() - 20, 50, 20); // 50x20
    lowSpeedButton.setBounds(midSpeedButton.getScreenX() - 50, midSpeedButton.getScreenY(), 50, 20);
    highSpeedButton.setBounds(midSpeedButton.getScreenX() + 50, midSpeedButton.getScreenY(), 50, 20);
    
    // Toggles
    midiToggle.setBounds(speedSlider.getScreenX() + speedSlider.getWidth()/3 - 15, speedSlider.getScreenY() + 70, 25, 20);
    tunedToggle.setBounds(speedSlider.getScreenX() + 2*speedSlider.getWidth()/3 - 15, midiToggle.getScreenY(), 25, 20);

    // Labels
    titleLabel.setBoundsToFit(getScreenBounds(),juce::Justification::topRight, true);
    
    // Let the generic controls layout first
    inspectButton.setSize(100, 50);
    inspectButton.setBoundsToFit(getScreenBounds(), juce::Justification::bottomRight, true);
}
