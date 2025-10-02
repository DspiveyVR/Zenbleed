#pragma once

#include "BinaryData.h"
#include "PluginProcessor.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include <juce_events/juce_events.h>

//==============================================================================
class PluginEditor
    : public juce::AudioProcessorEditor,
      public juce::AsyncUpdater
{
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    void handleAsyncUpdate() override
    {
        auto noteOff = processorRef.lastNoteSamplePos.load();
        logMessage("last note " + std::to_string(noteOff));
        repaint();
    }

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PluginProcessor& processorRef;
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "Inspect the UI" };

    juce::TextEditor midiMessagesBox;

    void logMessage(const juce::String& m)
    {
        midiMessagesBox.moveCaretToEnd();
        midiMessagesBox.insertTextAtCaret(m + juce::newLine);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
