#include "PluginEditor.h"

VocalSibilanceEditor::VocalSibilanceEditor (VocalSibilanceProcessor& p)
    : AudioProcessorEditor (p), processor (p), header (p, lnf), display (p)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (header);
    addAndMakeVisible (display);

    struct KnobSpec
    {
        vs::PorcelainKnob& knob;
        juce::Label& label;
        const char* id;
        const char* text;
        double resetTo;
    };
    const KnobSpec specs[] = {
        { smoothKnob,    smoothLabel,    "smooth",    "SMOOTH",     35.0 },
        { gritKnob,      gritLabel,      "grit",      "GRIT",        0.0 },
        { characterKnob, characterLabel, "character", "CHARACTER",  30.0 },
        { mixKnob,       mixLabel,       "mix",       "MIX",       100.0 },
        { outputKnob,    outputLabel,    "output",    "OUTPUT",      0.0 },
    };
    for (const auto& s : specs)
    {
        addAndMakeVisible (s.knob);
        s.knob.setDoubleClickReturnValue (true, s.resetTo);
        s.knob.setTitle (s.text);
        addAndMakeVisible (s.label);
        s.label.setText (s.text, juce::dontSendNotification);
        s.label.setJustificationType (juce::Justification::centred);
        s.label.setColour (juce::Label::textColourId, vs::porcelain::muted);
        s.label.setInterceptsMouseClicks (false, false);
    }

    // parent the drag-value bubbles to the editor so they stay inside the
    // plugin window (desktop-level popups can fight DAW focus handling)
    for (auto* k : { &smoothKnob, &gritKnob, &characterKnob, &mixKnob, &outputKnob })
        k->setPopupDisplayEnabled (true, false, this);

    smoothAtt    = std::make_unique<SliderAttachment> (p.apvts, "smooth",    smoothKnob);
    gritAtt      = std::make_unique<SliderAttachment> (p.apvts, "grit",      gritKnob);
    characterAtt = std::make_unique<SliderAttachment> (p.apvts, "character", characterKnob);
    mixAtt       = std::make_unique<SliderAttachment> (p.apvts, "mix",       mixKnob);
    outputAtt    = std::make_unique<SliderAttachment> (p.apvts, "output",    outputKnob);

    setResizable (true, true);
    setResizeLimits (312, 216, 1040, 720);                 // 60 % .. 200 %
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);

    const double scale = processor.apvts.state.getProperty ("uiScale", 1.0);
    setSize (juce::roundToInt (baseWidth * scale), juce::roundToInt (baseHeight * scale));
}

VocalSibilanceEditor::~VocalSibilanceEditor()
{
    setLookAndFeel (nullptr);
}

void VocalSibilanceEditor::paint (juce::Graphics& g)
{
    g.fillAll (vs::porcelain::face);
}

void VocalSibilanceEditor::resized()
{
    processor.apvts.state.setProperty ("uiScale", (double) getWidth() / baseWidth, nullptr);

    const float s = (float) getWidth() / baseWidth;
    auto r = getLocalBounds().reduced (juce::roundToInt (14 * s));

    header.setBounds (r.removeFromTop (juce::roundToInt (34 * s)));
    r.removeFromTop (juce::roundToInt (10 * s));
    display.setBounds (r.removeFromTop (juce::roundToInt (140 * s)));
    r.removeFromTop (juce::roundToInt (12 * s));

    juce::Label* labels[] = { &smoothLabel, &gritLabel, &characterLabel, &mixLabel, &outputLabel };
    vs::PorcelainKnob* knobs[] = { &smoothKnob, &gritKnob, &characterKnob, &mixKnob, &outputKnob };
    const int colW = r.getWidth() / 5;
    for (int i = 0; i < 5; ++i)
    {
        auto col = r.withX (r.getX() + i * colW).withWidth (colW);
        auto labelArea = col.removeFromBottom (juce::roundToInt (16 * s));
        labels[i]->setBounds (labelArea);
        labels[i]->setFont (juce::Font (juce::FontOptions (10.0f * s))
                                .withExtraKerningFactor (0.15f));
        const int kSize = juce::jmin (col.getWidth(), col.getHeight())
                          - juce::roundToInt (8 * s);
        knobs[i]->setBounds (col.withSizeKeepingCentre (kSize, kSize));
    }
}
