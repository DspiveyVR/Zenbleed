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

    // Speed Control
    juce::TextButton lowSpeedButton { "Low" };
    juce::TextButton midSpeedButton { "Mid" };
    juce::TextButton highSpeedButton { "High" };
    juce::Slider speedSlider;
    juce::RangedAudioParameter* currentSpeedParam = nullptr;

    // Labels
    juce::Label titleLabel;
    // NOFIX ;)
    juce::Label theLambel;

    // Toggles
    juce::ToggleButton midiToggle { "Midi Mode" };
    juce::ToggleButton tunedToggle { "Tuned Mode" };

    // Midi and Tuned mode sliders
    juce::Slider noteLengthSlider { "Note Length" };
    juce::Slider velocitySlider { "Velocity" };
    juce::ToggleButton keytrackToggle { "Keytrack" };
    juce::Slider fixedNoteSlider { "Fixed Note" };
    juce::Slider pitchBendSlider { "Pitch Bend" };

    // ETET Radio Controls
    // TODO: Convert manual toggles to radio buttons.
    juce::ToggleButton eTETMode { "Extratone Equal Temperament" };
    juce::Slider rootNoteSlider { "Root Note" };
    juce::Slider numeratorSlider { "Numerator" };
    juce::Slider denominatorSlider { "Denominator" };

    void openButtonClicked();
    void speedSliderButtonClicked(const juce::String&);
    PluginProcessor& getProcessor() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
