#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PorcelainLookAndFeel.h"

namespace vs
{
// Gentle animated rain on the editor face. Purely decorative: never
// intercepts the mouse, sits behind every control.
class RainBackground : public juce::Component, private juce::Timer
{
public:
    RainBackground()
    {
        setInterceptsMouseClicks (false, false);
        startTimerHz (30);
    }

    ~RainBackground() override { stopTimer(); }

    void resized() override
    {
        for (auto& d : drops)
            seed (d, true);
    }

    void paint (juce::Graphics& g) override
    {
        const float slant = -0.18f;
        for (const auto& d : drops)
        {
            g.setColour (porcelain::muted.withAlpha (d.alpha));
            g.drawLine (d.x, d.y, d.x + slant * d.len, d.y + d.len, 1.0f);
        }
    }

private:
    struct Drop { float x = 0, y = 0, len = 0, speed = 0, alpha = 0; };

    void seed (Drop& d, bool anywhere)
    {
        const float h = (float) juce::jmax (1, getHeight());
        d.x     = rng.nextFloat() * (float) getWidth();
        d.y     = anywhere ? rng.nextFloat() * h : -d.len;
        d.len   = h * (0.035f + rng.nextFloat() * 0.05f);
        d.speed = h * (0.12f + rng.nextFloat() * 0.22f);     // px per second
        d.alpha = 0.05f + rng.nextFloat() * 0.08f;           // very subtle
    }

    void timerCallback() override
    {
        const float dt = 1.0f / 30.0f;
        for (auto& d : drops)
        {
            d.y += d.speed * dt;
            if (d.y > (float) getHeight())
                seed (d, false);
        }
        repaint();
    }

    std::array<Drop, 48> drops;
    juce::Random rng;
};
} // namespace vs
