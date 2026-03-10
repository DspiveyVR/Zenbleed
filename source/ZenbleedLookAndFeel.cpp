#include "ZenbleedLookAndFeel.h"
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

const auto ACCENT_COLOR = juce::Colour(0xff00ffcc);

ZenbleedLookAndFeel::ZenbleedLookAndFeel() {
    // Background of the list
    setColour(juce::PopupMenu::backgroundColourId, juce::Colours::black);
    // Outline of the list
    setColour(juce::PopupMenu::headerTextColourId, juce::Colours::grey);
    // Arrow in the main box
    setColour(juce::ComboBox::arrowColourId, juce::Colours::white);
}

juce::Typeface::Ptr ZenbleedLookAndFeel::getTypefaceForFont(const juce::Font&) { return typeface; }

juce::Font ZenbleedLookAndFeel::getLabelFont(juce::Label& label) {
    return { juce::FontOptions(typeface).withHeight(label.getFont().getHeight()) };
}

juce::Font ZenbleedLookAndFeel::getTextButtonFont(juce::TextButton&, const int buttonHeight) {
    return { juce::FontOptions(typeface).withHeight(juce::jmin(16.0f, static_cast<float>(buttonHeight) * 0.6f)) };
}

juce::Font ZenbleedLookAndFeel::getComboBoxFont(juce::ComboBox& box) {
    return { juce::FontOptions(typeface).withHeight(juce::jmin(16.0f, static_cast<float>(box.getHeight()) * 0.85f)) };
}

void ZenbleedLookAndFeel::drawButtonBackground(
    juce::Graphics& g,
    juce::Button& button,
    const juce::Colour& backgroundColour,
    const bool shouldDrawButtonAsHighlighted,
    const bool shouldDrawButtonAsDown
) {
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);

    auto baseColour = juce::Colours::black.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                          .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

    if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted) {
        baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);
    }

    g.setColour(baseColour);

    // Squared background
    g.fillRect(bounds);

    // Squared outline
    g.setColour(button.findColour(juce::ComboBox::outlineColourId));
    g.drawRect(bounds, 1.0f);
}

void ZenbleedLookAndFeel::drawTickBox(
    juce::Graphics& g,
    juce::Component& component,
    float x, float y, float w, float h,
    const bool ticked,
    const bool isEnabled,
    const bool shouldDrawButtonAsHighlighted,
    const bool shouldDrawButtonAsDown
) {
    const auto bounds = juce::Rectangle<float>(x, y, w, h).reduced(0.5f, 0.5f);

    // Draw squared background for the tickbox
    g.setColour(juce::Colours::black);
    g.fillRect(bounds);

    // Draw squared outline
    g.setColour(component.findColour(juce::ComboBox::outlineColourId));
    g.drawRect(bounds, 1.0f);

    if (ticked) {
        g.setColour(isEnabled ? juce::Colours::white : juce::Colours::grey);
        const auto tickBounds = bounds.reduced(3.0f);
        // Drawing a simple squared "tick" inside
        g.fillRect(tickBounds);
    }
}

void ZenbleedLookAndFeel::drawToggleButton(
    juce::Graphics& g,
    juce::ToggleButton& button,
    const bool shouldDrawButtonAsHighlighted,
    const bool shouldDrawButtonAsDown
) {
    const auto fontSize = juce::jmin(15.0f, static_cast<float>(button.getHeight()) * 0.75f);
    const auto tickWidth = fontSize * 1.1f;

    drawTickBox(
        g,
        button,
        4.0f,
        (static_cast<float>(button.getHeight()) - tickWidth) * 0.5f,
        tickWidth,
        tickWidth,
        button.getToggleState(),
        button.isEnabled(),
        shouldDrawButtonAsHighlighted,
        shouldDrawButtonAsDown
    );

    g.setColour(button.findColour(juce::ToggleButton::textColourId));
    g.setFont(juce::Font(juce::FontOptions(typeface).withHeight(fontSize)));

    if (!button.isEnabled())
        g.setOpacity(0.5f);

    g.drawFittedText(
        button.getButtonText(),
        button.getLocalBounds().withTrimmedLeft(juce::roundToInt(tickWidth) + 10).withTrimmedRight(2),
        juce::Justification::centredLeft,
        10
    );
}

void ZenbleedLookAndFeel::drawLinearSlider(
    juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPos,
    float minSliderPos,
    float maxSliderPos,
    const juce::Slider::SliderStyle style,
    juce::Slider& slider
) {
    // 1. Define colors
    const auto trackColor = slider.findColour(juce::Slider::backgroundColourId).withAlpha(1.0f);
    const auto handleColor = juce::Colours::red; // You can also use juce::Colour(0xffff4444) for a nicer red
    const auto outlineColor = slider.findColour(juce::Slider::textBoxOutlineColourId);

    auto isTwoValue =
        (style == juce::Slider::SliderStyle::TwoValueVertical
         || style == juce::Slider::SliderStyle::TwoValueHorizontal);
    auto isThreeValue =
        (style == juce::Slider::SliderStyle::ThreeValueVertical
         || style == juce::Slider::SliderStyle::ThreeValueHorizontal);

    // 2. Draw the Track
    g.setColour(trackColor);
    auto trackWidth = 4.0f;

    if (slider.isHorizontal()) {
        g.fillRoundedRectangle(x, y + height * 0.5f - trackWidth * 0.5f, width, trackWidth, 2.0f);
    } else {
        g.fillRoundedRectangle(x + width * 0.5f - trackWidth * 0.5f, y, trackWidth, height, 2.0f);
    }

    // 3. Draw the Handle (The Red Part)
    g.setColour(handleColor);

    float handleSize = 12.0f;
    juce::Rectangle<float> thumb;

    if (slider.isHorizontal()) {
        thumb.setBounds(sliderPos - handleSize * 0.5f, y + height * 0.5f - handleSize * 0.5f, handleSize, handleSize);
    } else {
        thumb.setBounds(x + width * 0.5f - handleSize * 0.5f, sliderPos - handleSize * 0.5f, handleSize, handleSize);
    }

    // Drawing a nice circular red handle
    g.fillEllipse(thumb);

    // Optional: Add a subtle outline to the handle to make it pop
    g.setColour(outlineColor.withAlpha(0.5f));
    g.drawEllipse(thumb, 1.0f);
}

void ZenbleedLookAndFeel::drawComboBox(
    juce::Graphics& g,
    int width,
    int height,
    bool isButtonDown,
    int buttonX,
    int buttonY,
    int buttonW,
    int buttonH,
    juce::ComboBox& box
) {
    const auto bounds = box.getLocalBounds().toFloat().reduced(0.5f, 0.5f);

    // 1. Draw the Black Background (Squared)
    g.setColour(juce::Colours::black);
    g.fillRect(bounds);

    // 2. Draw the Outline (Squared)
    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRect(bounds, 1.0f);

    // 3. Draw the Arrow
    juce::Path path;
    const float arrowW = 8.0f;
    const float arrowH = 5.0f;
    const float centerX = static_cast<float>(width) - arrowW - 8.0f;
    const float centerY = static_cast<float>(height) * 0.5f;

    path.startNewSubPath(centerX, centerY + arrowH * 0.5f);
    path.lineTo(centerX + arrowW * 0.5f, centerY - arrowH * 0.5f);
    path.lineTo(centerX + arrowW, centerY + arrowH * 0.5f);

    g.setColour(box.findColour(juce::ComboBox::arrowColourId).withAlpha(box.isEnabled() ? 0.9f : 0.2f));
    g.strokePath(path, juce::PathStrokeType(1.5f));
}
