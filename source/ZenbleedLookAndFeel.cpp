#include "ZenbleedLookAndFeel.h"
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

const auto ACCENT_COLOR = juce::Colour(0xff00ffcc);

ZenbleedLookAndFeel::ZenbleedLookAndFeel() {
}

juce::Typeface::Ptr ZenbleedLookAndFeel::getTypefaceForFont(const juce::Font&) {
    return typeface;
}

juce::Font ZenbleedLookAndFeel::getLabelFont(juce::Label& label) {
    return {juce::FontOptions(typeface).withHeight(label.getFont().getHeight())};
}

juce::Font ZenbleedLookAndFeel::getTextButtonFont(juce::TextButton&, const int buttonHeight) {
    return {juce::FontOptions(typeface).withHeight(juce::jmin(16.0f, static_cast<float>(buttonHeight) * 0.6f))};
}

juce::Font ZenbleedLookAndFeel::getComboBoxFont(juce::ComboBox& box) {
    return {juce::FontOptions(typeface).withHeight(juce::jmin(16.0f, static_cast<float>(box.getHeight()) * 0.85f))};
}

void ZenbleedLookAndFeel::drawButtonBackground(
    juce::Graphics& g,
    juce::Button& button,
    const juce::Colour& backgroundColour,
    const bool shouldDrawButtonAsHighlighted,
    const bool shouldDrawButtonAsDown
) {
    constexpr auto cornerSize = 6.0f;
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);

    auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                          .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

    if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted) {
        baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);
    }

    g.setColour(baseColour);

    const auto flatOnLeft = button.isConnectedOnLeft();
    const auto flatOnRight = button.isConnectedOnRight();
    const auto flatOnTop = button.isConnectedOnTop();
    const auto flatOnBottom = button.isConnectedOnBottom();

    if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom) {
        juce::Path path;
        path.addRoundedRectangle(
            bounds.getX(),
            bounds.getY(),
            bounds.getWidth(),
            bounds.getHeight(),
            cornerSize,
            cornerSize,
            !(flatOnLeft || flatOnTop),
            !(flatOnRight || flatOnTop),
            !(flatOnLeft || flatOnBottom),
            !(flatOnRight || flatOnBottom)
        );

        g.fillPath(path);

        g.setColour(button.findColour(juce::ComboBox::outlineColourId));
        g.strokePath(path, juce::PathStrokeType(1.0f));
    } else {
        g.fillRoundedRectangle(bounds, cornerSize);

        g.setColour(button.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
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

    if (!button.isEnabled()) g.setOpacity(0.5f);

    g.drawFittedText(
        button.getButtonText(),
        button.getLocalBounds().withTrimmedLeft(juce::roundToInt(tickWidth) + 10).withTrimmedRight(2),
        juce::Justification::centredLeft,
        10
    );
}
