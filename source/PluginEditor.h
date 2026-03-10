#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "ZenbleedLookAndFeel.h"
#include "melatonin_inspector/melatonin_inspector.h"

class PluginEditor final : public juce::AudioProcessorEditor, public juce::AudioProcessorValueTreeState::Listener {
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    ZenbleedLookAndFeel lookAndFeel;
    PluginProcessor& processorRef;
    std::unique_ptr<melatonin::Inspector> inspector;
    std::unique_ptr<juce::FileChooser> chooser;

    // UI Buttons
    juce::TextButton inspectButton { "Inspect the UI" };
    juce::TextButton openButton { "Open" };
    juce::Label sampleNameLabel; // Shows "No sample loaded" or filename

    // Speed Control (Manual Radio Group)
    juce::TextButton lowSpeedButton { "Low" };
    juce::TextButton midSpeedButton { "Mid" };
    juce::TextButton highSpeedButton { "High" };
    juce::Slider speedSlider;
    juce::Label speedValueLabel;
    juce::Label speedName;

    // Toggles
    juce::ToggleButton midiToggle { "Midi Mode" };
    juce::ToggleButton keytrackToggle { "Keytrack" };

    // Sliders
    juce::Slider noteLengthSlider, velocitySlider, fixedNoteSlider, pitchBendSlider;
    juce::Slider rootNoteSlider, numeratorSlider, denominatorSlider;

    // Value Labels (Dynamic readouts)
    juce::Label noteLengthLabel, velocityLabel, fixedNoteLabel, pitchBendLabel;
    juce::Label rootNoteLabel, numeratorLabel, denominatorLabel;

    // Name Labels (Static text)
    juce::Label noteLengthName, velocityName, fixedNoteName, pitchBendName;
    juce::Label rootNoteName, numeratorName, denominatorName;

    // Static Header Labels
    juce::Label titleLabel;
    juce::Label theLambel;

    juce::ImageComponent logoComponent;

    // Attachments
    std::unique_ptr<SliderAttachment> speedAttachment;
    std::unique_ptr<ButtonAttachment> midiToggleAttachment;
    std::unique_ptr<ButtonAttachment> keytrackAttachment;

    std::unique_ptr<SliderAttachment> noteLengthAttachment;
    std::unique_ptr<SliderAttachment> velocityAttachment;
    std::unique_ptr<SliderAttachment> fixedNoteAttachment;
    std::unique_ptr<SliderAttachment> pitchBendAttachment;
    std::unique_ptr<SliderAttachment> rootNoteAttachment;
    std::unique_ptr<SliderAttachment> numeratorAttachment;
    std::unique_ptr<SliderAttachment> denominatorAttachment;
    std::unique_ptr<ComboBoxAttachment> modeAttachment;

    juce::ComboBox modeSelector;
    juce::Label modeLabel;

    const float originalWidth = 700.0f;
    const float originalHeight = 520.0f;

    juce::ComponentBoundsConstrainer constrainer;

    juce::Image logo;
    juce::Image background;

    // Helper functions
    void openButtonClicked();
    void speedSliderButtonClicked(const juce::String& parameterID);
    void updateLabelText(juce::Label& label, juce::Slider& slider, juce::String suffix = "");
    PluginProcessor& getProcessor() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
