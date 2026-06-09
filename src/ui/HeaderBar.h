#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "../PluginProcessor.h"
#include "../state/PresetManager.h"
#include "PorcelainLookAndFeel.h"

namespace vs
{
// Header: wordmark | preset menu | LISTEN pill | DARARARA | soft-bypass dot.
class HeaderBar : public juce::Component
{
public:
    HeaderBar (VocalSibilanceProcessor& proc, PorcelainLookAndFeel& lookAndFeel);

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    // Filled ember dot while processing; hollow outline while bypassed.
    class PowerDot : public juce::ToggleButton
    {
    public:
        void paintButton (juce::Graphics& g, bool, bool) override
        {
            const auto r = getLocalBounds().toFloat();
            const float d = juce::jmin (r.getWidth(), r.getHeight()) - 4.0f;
            const auto c = r.getCentre();
            if (getToggleState())   // toggle ON == bypassed
            {
                g.setColour (porcelain::line);
                g.drawEllipse (c.x - d / 2, c.y - d / 2, d, d, 1.5f);
            }
            else
            {
                g.setColour (porcelain::accent);
                g.fillEllipse (c.x - d / 2, c.y - d / 2, d, d);
            }
        }
    };

    void presetChosen();

    VocalSibilanceProcessor& processor;
    PorcelainLookAndFeel& lnf;
    PresetManager presets;

    juce::ComboBox presetBox;
    juce::TextButton listenButton { "LISTEN" };
    PowerDot bypassDot;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> listenAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
};
} // namespace vs
