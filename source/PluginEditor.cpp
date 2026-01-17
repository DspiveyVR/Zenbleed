#include "PluginEditor.h"

PluginEditor::PluginEditor(PluginProcessor& p) : GenericAudioProcessorEditor(&p), processorRef(p) {
    // addAndMakeVisible(inspectButton);

    // // this chunk of code instantiates and opens the melatonin inspector
    // inspectButton.onClick = [&] {
    //     if(!inspector)
    //     {
    //         inspector = std::make_unique<melatonin::Inspector>(*this);
    //         inspector->onClose = [this]() { inspector.reset(); };
    //     }

    //     inspector->setVisible(true);
    // };

    const juce::StringArray speedRanges = processorRef.getSpeedRangeChoices();
    for (size_t i = 0; i < speedRanges.size(); i++) {
        speedRangeBox.addItem(speedRanges[i], i + 1);
    }

    speedRangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processorRef.getParametersApvts(),
            "speedRange",
            speedRangeBox
    );

    addAndMakeVisible(openButton);
    addAndMakeVisible(speedRangeBox);

    openButton.onClick = [this] { openButtonClicked(); };

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(640, 480);
}

PluginEditor::~PluginEditor() {}

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

PluginProcessor& PluginEditor::getProcessor() const { return static_cast<PluginProcessor&>(processor); }

void PluginEditor::paint(juce::Graphics& g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    // auto area = getLocalBounds();
    // g.setColour(juce::Colours::white);
    // g.setFont(16.0f);
    // auto helloWorld = juce::String("Hello from ") + PRODUCT_NAME_WITHOUT_VERSION + " v" VERSION + " running in " + CMAKE_BUILD_TYPE;
    // g.drawText(helloWorld, area.removeFromTop(150), juce::Justification::centred, false);
    juce::GenericAudioProcessorEditor::paint(g);
}

void PluginEditor::resized() {
    // layout the positions of your child components here
    juce::GenericAudioProcessorEditor::resized();
    openButton.setBounds((getWidth() / 2) - (100 / 2), 150, 100, 40);
    speedRangeBox.setBounds((getWidth() / 2) - (100 / 2), 200, 100, 40);
    // Let the generic controls layout first
    // inspectButton.setBounds(getLocalBounds().withSizeKeepingCentre(100, 50));
}
