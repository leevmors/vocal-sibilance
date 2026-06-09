#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace vs
{
namespace porcelain
{
    const juce::Colour face    { 0xFFF7F6F3 };
    const juce::Colour inset   { 0xFFEFEDE8 };
    const juce::Colour line    { 0xFFDCD9D2 };
    const juce::Colour barIdle { 0xFFD5D2CA };
    const juce::Colour text    { 0xFF2A2A28 };
    const juce::Colour muted   { 0xFF6B6862 };
    const juce::Colour accent  { 0xFFE0552C };
}

class PorcelainLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PorcelainLookAndFeel();

    juce::Font labelFont (float height) const;
    juce::Font titleFont (float height) const;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

private:
    juce::Typeface::Ptr regular, semiBold;
};
} // namespace vs
