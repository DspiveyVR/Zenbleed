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

    void drawToggleButton(
        juce::Graphics& g,
        juce::ToggleButton& button,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown
    ) override;

    void drawLinearSlider(
        juce::Graphics&,
        int x,
        int y,
        int width,
        int height,
        float sliderPos,
        float minSliderPos,
        float maxSliderPos,
        const juce::Slider::SliderStyle,
        juce::Slider&
    ) override;

    void drawComboBox(
        juce::Graphics&,
        int width,
        int height,
        bool isButtonDown,
        int buttonX,
        int buttonY,
        int buttonW,
        int buttonH,
        juce::ComboBox&
    ) override;

    void drawTickBox(
        juce::Graphics& g,
        juce::Component& component,
        float x,
        float y,
        float w,
        float h,
        const bool ticked,
        const bool isEnabled,
        const bool shouldDrawButtonAsHighlighted,
        const bool shouldDrawButtonAsDown
    ) override;

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font&) override;

private:
    juce::Typeface::Ptr typeface =
        juce::Typeface::createSystemTypefaceFor(BinaryData::font_ttf, BinaryData::font_ttfSize);
};
