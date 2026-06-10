#pragma once
#include <array>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PorcelainLookAndFeel.h"

namespace vs
{
// Ember pixel-rain on the editor face. Purely decorative: never intercepts
// the mouse, sits behind every control. Positions are normalized so window
// resizes rescale the rain instead of resetting it.
class RainBackground : public juce::Component, private juce::Timer
{
public:
    RainBackground()
    {
        setInterceptsMouseClicks (false, false);
        for (auto& d : drops)
            seed (d, true);
        startTimerHz (30);
    }

    ~RainBackground() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        const float w = (float) getWidth(), h = (float) getHeight();
        const int unit = juce::jmax (2, juce::roundToInt (w / 240.0f));
        for (const auto& d : drops)
        {
            const int px = unit * d.size;
            const int gx = ((int) (d.xNorm * w) / unit) * unit;   // pixel grid snap
            const int gy = ((int) (d.yNorm * h) / unit) * unit;
            g.setColour (porcelain::accent.withAlpha (d.alpha));
            g.fillRect (gx, gy, px, d.tall ? px * 2 : px);
        }
    }

private:
    struct Drop
    {
        float xNorm = 0, yNorm = 0, speedNorm = 0, alpha = 0;
        int size = 1;
        bool tall = false;
    };

    void seed (Drop& d, bool anywhere)
    {
        d.xNorm     = rng.nextFloat();
        d.yNorm     = anywhere ? rng.nextFloat() : -0.03f;
        d.speedNorm = 0.30f + rng.nextFloat() * 0.35f;   // heights/second: real rain
        d.alpha     = 0.10f + rng.nextFloat() * 0.18f;
        d.size      = rng.nextFloat() < 0.7f ? 1 : 2;
        d.tall      = rng.nextFloat() < 0.35f;
    }

    void timerCallback() override
    {
        const float dt = 1.0f / 30.0f;
        for (auto& d : drops)
        {
            d.yNorm += d.speedNorm * dt;
            if (d.yNorm > 1.0f)
                seed (d, false);
        }
        repaint();
    }

    std::array<Drop, 44> drops;
    juce::Random rng;
};
} // namespace vs
