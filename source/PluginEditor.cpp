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

    const juce::StringArray speedRanges = processorRef.getSpeedRangeChoices();
    for (size_t i = 0; i < speedRanges.size(); i++) {
        speedRangeBox.addItem(speedRanges[i], i + 1);
    }

    speedRangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getParametersApvts(), "speedRange", speedRangeBox
    );

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    // NOTE: The size must be set, *before* we addAndMakeVisible.
    setSize(640, 480);

    titleLabel.setText("Zenbleed", juce::dontSendNotification);
    titleFrame.addAndMakeVisible(titleLabel);

    addAndMakeVisible(titleFrame);

    lowerFrame.addAndMakeVisible(openButton);
    lowerFrame.addAndMakeVisible(speedRangeBox);

    controlFrame.addAndMakeVisible(lowSpeedButton);
    controlFrame.addAndMakeVisible(midSpeedButton);
    controlFrame.addAndMakeVisible(highSpeedButton);
    controlFrame.addAndMakeVisible(speedSlider);

    featureFrame.addAndMakeVisible(midiToggle);
    featureFrame.addAndMakeVisible(tunedToggle);

    addAndMakeVisible(lowerFrame);
    addAndMakeVisible(controlFrame);
    addAndMakeVisible(featureFrame);

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
    auto area = getLocalBounds();

    // Split `area` into proper regions.
    auto titleArea = area.removeFromTop(50);
    titleFrame.setBounds(titleArea);
    auto leftArea = area.removeFromLeft(area.getWidth() / 2);
    auto rightArea = area;
    auto featureArea = leftArea.removeFromTop(leftArea.getHeight() / 2);
    auto lowerArea = leftArea;

    featureFrame.setBounds(featureArea);
    lowerFrame.setBounds(lowerArea);
    controlFrame.setBounds(rightArea);

    // Title Frame
    titleLabel.setBounds(titleFrame.getLocalBounds().reduced(10).removeFromRight(100));

    // Lower Frame
    {
        auto lf = lowerFrame.getLocalBounds().reduced(10);
        openButton.setBounds(lf.removeFromLeft(110));
        openButton.setSize(100, 50);
        speedRangeBox.setBounds(lf.removeFromLeft(110));
        speedRangeBox.setSize(100, 50);
        // speedRangeBox.setCentrePosition(speedRangeBox.getBounds().getCentre());
    }

    // Control Frame
    {
        auto cf = controlFrame.getLocalBounds();

        // Widget Sizing
        const int buttonWidth = 80;
        const int buttonHeight = 30;
        const int buttonGap = 10;

        const int sliderWidth = 300;
        const int sliderHeight = 40;
        const int verticalGap = 15;

        const int totalWidth = buttonWidth * 3 + buttonGap * 2;

        const int totalHeight = buttonHeight + verticalGap + sliderHeight;

        // Selecting buttons and slider as a group.
        auto group = cf.withSizeKeepingCentre(totalWidth, totalHeight);

        // Defining the row of buttons.
        auto buttonRow = group.removeFromTop(buttonHeight);

        lowSpeedButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
        buttonRow.removeFromLeft(buttonGap);

        midSpeedButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
        buttonRow.removeFromLeft(buttonGap);

        highSpeedButton.setBounds(buttonRow.removeFromLeft(buttonWidth));

        // Add space between the buttons and slider.
        group.removeFromTop(verticalGap);

        // Center the slider under the buttons.
        speedSlider.setBounds(group.removeFromTop(sliderHeight).withSizeKeepingCentre(sliderWidth, sliderHeight));
    }

    // Feature Frame
    {
        auto ff = featureFrame.getLocalBounds().reduced(10);
        midiToggle.setBounds(ff.removeFromTop(30));
        tunedToggle.setBounds(ff.removeFromTop(30));
    }

    // Inspect Button
    inspectButton.setSize(100, 50);
    inspectButton.setAlwaysOnTop(true);
    inspectButton.setBoundsToFit(getScreenBounds(), juce::Justification::bottomRight, true);
}
