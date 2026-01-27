#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class ZenbleedLookAndFeel : public juce::LookAndFeel_V4 {
public:
    ZenbleedLookAndFeel() { setColour(juce::Slider::thumbColourId, juce::Colours::red); }

    void drawButtonBackground(
        juce::Graphics&,
        juce::Button&,
        const juce::Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown
    ) override;
};
