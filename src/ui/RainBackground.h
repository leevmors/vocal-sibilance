#pragma once
#include <array>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PorcelainLookAndFeel.h"

namespace vs
{
// Gentle ember pixel-rain on the editor face. Purely decorative: never
// intercepts the mouse, sits behind every control. Hard-edged little
// squares on a snapped grid give it the pixel-art feel.
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
        const int unit = juce::jmax (2, juce::roundToInt ((float) getWidth() / 240.0f));
        for (const auto& d : drops)
        {
            const int px = unit * d.size;
            const int gx = ((int) d.x / unit) * unit;    // snap to the pixel grid
            const int gy = ((int) d.y / unit) * unit;
            g.setColour (porcelain::accent.withAlpha (d.alpha));
            g.fillRect (gx, gy, px, d.tall ? px * 2 : px);
        }
    }

private:
    struct Drop
    {
        float x = 0, y = 0, speed = 0, alpha = 0;
        int size = 1;
        bool tall = false;
    };

    void seed (Drop& d, bool anywhere)
    {
        const float h = (float) juce::jmax (1, getHeight());
        d.x     = rng.nextFloat() * (float) getWidth();
        d.y     = anywhere ? rng.nextFloat() * h : -8.0f;
        d.speed = h * (0.10f + rng.nextFloat() * 0.20f);     // px per second
        d.alpha = 0.10f + rng.nextFloat() * 0.18f;           // subtle ember
        d.size  = rng.nextFloat() < 0.7f ? 1 : 2;
        d.tall  = rng.nextFloat() < 0.35f;
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

    std::array<Drop, 44> drops;
    juce::Random rng;
};
} // namespace vs
