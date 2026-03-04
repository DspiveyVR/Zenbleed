#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

class ZenbleedLookAndFeel : public juce::LookAndFeel_V4 {
public:
    ZenbleedLookAndFeel();

    juce::Font getLabelFont(juce::Label& label) override;
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;

    void drawButtonBackground(
        juce::Graphics& g,
        juce::Button&,
        const juce::Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown
    ) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font&) override;
private:
    juce::Typeface::Ptr typeface =
        juce::Typeface::createSystemTypefaceFor(BinaryData::McLarenRegular_ttf, BinaryData::McLarenRegular_ttfSize);
};
