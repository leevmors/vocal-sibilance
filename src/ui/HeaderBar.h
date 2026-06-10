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

    // Brand wordmark that links to the dararara channel.
    class BrandLink : public juce::Component
    {
    public:
        explicit BrandLink (PorcelainLookAndFeel& l) : lnf (l)
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            setTitle ("bydararara on YouTube");
        }
        void paint (juce::Graphics& g) override
        {
            g.setColour (hover ? porcelain::accent : porcelain::muted);
            g.setFont (lnf.labelFont ((float) getHeight() * 0.72f)
                           .withExtraKerningFactor (0.20f));
            g.drawText ("BYDARARARA", getLocalBounds(), juce::Justification::centredRight);
        }
        void mouseEnter (const juce::MouseEvent&) override { hover = true;  repaint(); }
        void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }
        void mouseUp (const juce::MouseEvent& e) override
        {
            if (e.mouseWasClicked())
                juce::URL ("https://www.youtube.com/@darararabeats").launchInDefaultBrowser();
        }
    private:
        PorcelainLookAndFeel& lnf;
        bool hover = false;
    };

    void presetChosen();

    VocalSibilanceProcessor& processor;
    PorcelainLookAndFeel& lnf;
    PresetManager presets;

    juce::ComboBox presetBox;
    juce::TextButton listenButton { "LISTEN" };
    PowerDot bypassDot;
    BrandLink brand { lnf };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> listenAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
};
} // namespace vs
