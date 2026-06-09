#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace vs
{
// Rotary slider with porcelain styling hooks. Its caption label is owned and
// drawn by the editor; the value readout is the drag-only popup bubble.
class PorcelainKnob : public juce::Slider
{
public:
    explicit PorcelainKnob (bool emphasized = false)
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        setPopupDisplayEnabled (true, false, nullptr);   // readout only while dragging
        getProperties().set ("emphasized", emphasized);
        setMouseDragSensitivity (180);                   // Shift-drag = fine (built in)
    }
};
} // namespace vs
