#pragma once

#include <juce_events/juce_events.h>
#include "PluginProcessor.h"
#include "melatonin_inspector/melatonin_inspector.h"

class PluginEditor final : public juce::GenericAudioProcessorEditor {
public:
    explicit PluginEditor(PluginProcessor&);

    ~PluginEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;

    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PluginProcessor& processorRef;
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "Inspect the UI" };
    juce::TextButton openButton { "Open" }; /**< Opens the file explorer to allow the user to select an audio sample. */
    std::unique_ptr<juce::FileChooser> chooser;

    void openButtonClicked();
    PluginProcessor& getProcessor() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
