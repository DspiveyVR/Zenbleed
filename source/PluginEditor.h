#pragma once

#include <juce_events/juce_events.h>
#include "PluginProcessor.h"
#include "ZenbleedLookAndFeel.h"
#include "melatonin_inspector/melatonin_inspector.h"

class PluginEditor final : public juce::AudioProcessorEditor {
public:
    explicit PluginEditor(PluginProcessor&);

    ~PluginEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;

    void resized() override;

private:
    ZenbleedLookAndFeel lookAndFeel;

    PluginProcessor& processorRef;
    std::unique_ptr<melatonin::Inspector> inspector;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::TextButton inspectButton { "Inspect the UI" };
    juce::TextButton openButton { "Open" }; /**< Opens the file explorer to allow the user to select an audio sample. */
    juce::ComboBox speedRangeBox;

    // Speed Control
    juce::TextButton lowSpeedButton { "Low" };
    juce::TextButton midSpeedButton { "Mid" };
    juce::TextButton highSpeedButton { "High" };
    juce::Slider speedSlider;
    juce::RangedAudioParameter* currentSpeedParam = nullptr;

    // Labels
    juce::Label titleLabel;
    juce::Label midiToggleLabel;
    juce::Label tunedToggleLabel;
    // TODO: Add labels to other parts of the window.

    // Frames
    juce::Component titleFrame { "Title Frame" };
    juce::Component featureFrame { "Feature Frame" };
    juce::Component controlFrame { "Control Frame" };
    juce::Component lowerFrame { "Lower Frame" };

    // Toggles
    juce::ToggleButton midiToggle { "Midi Mode" };
    juce::ToggleButton tunedToggle { "Tuned Mode" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> speedRangeAttachment;

    void openButtonClicked();
    void speedSliderButtonClicked(const juce::String&);
    PluginProcessor& getProcessor() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
