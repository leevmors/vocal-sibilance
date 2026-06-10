#include "PorcelainLookAndFeel.h"
#include "BinaryData.h"

namespace vs
{
PorcelainLookAndFeel::PorcelainLookAndFeel()
{
    regular  = juce::Typeface::createSystemTypefaceFor (BinaryData::InterRegular_ttf,
                                                        BinaryData::InterRegular_ttfSize);
    semiBold = juce::Typeface::createSystemTypefaceFor (BinaryData::InterSemiBold_ttf,
                                                        BinaryData::InterSemiBold_ttfSize);
    setDefaultSansSerifTypeface (regular);

    setColour (juce::ResizableWindow::backgroundColourId, porcelain::face);
    setColour (juce::Label::textColourId, porcelain::text);
    setColour (juce::PopupMenu::backgroundColourId, porcelain::face);
    setColour (juce::PopupMenu::textColourId, porcelain::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, porcelain::inset);
    setColour (juce::PopupMenu::highlightedTextColourId, porcelain::text);
    setColour (juce::ComboBox::backgroundColourId, porcelain::face);
    setColour (juce::ComboBox::textColourId, porcelain::muted);
    setColour (juce::ComboBox::outlineColourId, porcelain::line);
    setColour (juce::ComboBox::arrowColourId, porcelain::muted);
    setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    setColour (juce::TextButton::textColourOffId, porcelain::muted);
    setColour (juce::TextButton::textColourOnId, porcelain::accent);
    setColour (juce::BubbleComponent::backgroundColourId, porcelain::face);
    setColour (juce::BubbleComponent::outlineColourId, porcelain::line);
    setColour (juce::TooltipWindow::textColourId, porcelain::text);
    setColour (juce::TooltipWindow::backgroundColourId, porcelain::face);
    setColour (juce::TooltipWindow::outlineColourId, porcelain::line);
}

juce::Font PorcelainLookAndFeel::labelFont (float height) const
{
    return juce::Font (juce::FontOptions (regular).withHeight (height));
}

juce::Font PorcelainLookAndFeel::titleFont (float height) const
{
    return juce::Font (juce::FontOptions (semiBold).withHeight (height));
}

void PorcelainLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y,
                                             int width, int height, float pos,
                                             float startAngle, float endAngle,
                                             juce::Slider& slider)
{
    const auto rawBounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const float margin = juce::jmax (8.0f, juce::jmin (rawBounds.getWidth(), rawBounds.getHeight()) * 0.14f);
    const auto bounds = rawBounds.reduced (margin);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    const float radius = size * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    const bool emphasized = (bool) slider.getProperties().getWithDefault ("emphasized", false);
    const bool engaged = slider.isMouseOverOrDragging();

    // soft drop shadow grounds the knob
    {
        juce::Path body;
        body.addEllipse (centre.x - radius, centre.y - radius, size, size);
        juce::DropShadow (juce::Colour (0x26000000),
                          juce::jmax (3, juce::roundToInt (radius * 0.16f)),
                          { 0, juce::jmax (2, juce::roundToInt (radius * 0.06f)) })
            .drawForPath (g, body);
    }

    // ceramic body: top-lit gradient
    {
        juce::ColourGradient body (juce::Colour (0xFFFFFEFB),
                                   centre.x - radius * 0.35f, centre.y - radius * 0.55f,
                                   juce::Colour (0xFFEAE7E0),
                                   centre.x + radius * 0.45f, centre.y + radius * 0.95f,
                                   true);
        g.setGradientFill (body);
        g.fillEllipse (centre.x - radius, centre.y - radius, size, size);
    }

    // rim light hints at curvature
    g.setColour (juce::Colours::white.withAlpha (0.65f));
    g.drawEllipse (centre.x - radius + 1.2f, centre.y - radius + 1.2f,
                   size - 2.4f, size - 2.4f, 1.0f);

    g.setColour (emphasized ? porcelain::text : porcelain::line);   // outer ring
    g.drawEllipse (centre.x - radius, centre.y - radius, size, size,
                   emphasized ? 2.0f : 1.5f);

    if (engaged)                                                  // live-value arc
    {
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, radius + 3.0f, radius + 3.0f,
                           0.0f, startAngle, angle, true);
        g.setColour (porcelain::accent);
        g.strokePath (arc, juce::PathStrokeType (2.0f));
    }

    juce::Path pointer;                                           // notch
    pointer.addRoundedRectangle (-1.25f, -radius + 3.0f, 2.5f, radius * 0.35f, 1.0f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));
    g.setColour (porcelain::text);
    g.fillPath (pointer);
}

void PorcelainLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                                 const juce::Colour&,
                                                 bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const float corner = r.getHeight() * 0.5f;
    const bool on = b.getToggleState();

    g.setColour (on ? porcelain::accent.withAlpha (0.12f) : porcelain::face);
    g.fillRoundedRectangle (r, corner);
    g.setColour (on ? porcelain::accent
                    : (highlighted || down ? porcelain::muted : porcelain::line));
    g.drawRoundedRectangle (r, corner, 1.2f);
}

void PorcelainLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                         bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);
    g.setColour (porcelain::face);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (porcelain::line);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);

    juce::Path arrow;
    const float ax = (float) width - 14.0f, ay = (float) height * 0.5f;
    arrow.addTriangle (ax - 3.5f, ay - 2.0f, ax + 3.5f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (porcelain::muted);
    g.fillPath (arrow);
}
} // namespace vs
