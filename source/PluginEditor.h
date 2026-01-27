#pragma once

#include <juce_events/juce_events.h>
#include "PluginProcessor.h"
#include "ZenbleedLookAndFeel.h"
#include "melatonin_inspector/melatonin_inspector.h"

class PluginEditor final : public juce::GenericAudioProcessorEditor {
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
    
    // Text buttons for knob control
    juce::TextButton lowSpeed { "Low" };
    juce::TextButton midSpeed { "Mid" };
    juce::TextButton highSpeed { "High" };
    juce::Slider speedKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> speedRangeAttachment;

    void openButtonClicked();
    void speedKnobButtonClicked();
    PluginProcessor& getProcessor() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
