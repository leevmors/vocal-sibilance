#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "../PluginProcessor.h"
#include "PorcelainLookAndFeel.h"
#include "PorcelainKnob.h"
#include "SibilanceDisplay.h"
#include "HeaderBar.h"

class VocalSibilanceEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalSibilanceEditor (VocalSibilanceProcessor&);
    ~VocalSibilanceEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    VocalSibilanceProcessor& processor;
    vs::PorcelainLookAndFeel lnf;

    vs::HeaderBar header;
    vs::SibilanceDisplay display;

    vs::PorcelainKnob smoothKnob { true };   // emphasized
    vs::PorcelainKnob gritKnob, characterKnob, mixKnob, outputKnob;
    juce::Label smoothLabel, gritLabel, characterLabel, mixLabel, outputLabel;

    std::unique_ptr<SliderAttachment> smoothAtt, gritAtt, characterAtt, mixAtt, outputAtt;

    static constexpr int baseWidth = 520, baseHeight = 360;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalSibilanceEditor)
};
