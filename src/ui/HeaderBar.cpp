#include "HeaderBar.h"

namespace vs
{
HeaderBar::HeaderBar (VocalSibilanceProcessor& proc, PorcelainLookAndFeel& lookAndFeel)
    : processor (proc), lnf (lookAndFeel), presets (proc.apvts)
{
    addAndMakeVisible (presetBox);
    presetBox.setTextWhenNothingSelected ("presets");
    int id = 1;
    for (const auto& name : presets.getFactoryNames())
        presetBox.addItem (name, id++);
    presetBox.addSeparator();
    presetBox.addItem ("Save preset...", 100);
    presetBox.addItem ("Load preset...", 101);
    presetBox.onChange = [this] { presetChosen(); };

    addAndMakeVisible (listenButton);
    listenButton.setClickingTogglesState (true);
    listenAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, "listen", listenButton);

    addAndMakeVisible (bypassDot);
    bypassDot.setTitle ("Bypass");
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, "bypass", bypassDot);
}

void HeaderBar::presetChosen()
{
    const int id = presetBox.getSelectedId();
    if (id == 0)
        return;
    if (id < 100)
    {
        presets.applyFactory (id - 1);
        return;
    }

    presetBox.setSelectedId (0, juce::dontSendNotification);   // action, not a state
    juce::Component::SafePointer<HeaderBar> safeThis (this);
    const auto dir = PresetManager::userPresetDirectory();

    if (id == 100)
    {
        chooser = std::make_unique<juce::FileChooser> ("Save preset", dir, "*.vsib");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                  | juce::FileBrowserComponent::canSelectFiles,
            [safeThis] (const juce::FileChooser& fc)
            {
                if (safeThis == nullptr)
                    return;
                const auto f = fc.getResult();
                if (f != juce::File())
                    safeThis->presets.saveUserPreset (f.withFileExtension ("vsib"));
            });
    }
    else if (id == 101)
    {
        chooser = std::make_unique<juce::FileChooser> ("Load preset", dir, "*.vsib");
        chooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
            [safeThis] (const juce::FileChooser& fc)
            {
                if (safeThis == nullptr)
                    return;
                const auto f = fc.getResult();
                if (f.existsAsFile() && ! safeThis->presets.loadUserPreset (f))
                    safeThis->presetBox.setText ("preset load failed", juce::dontSendNotification);
            });
    }
}

void HeaderBar::resized()
{
    auto r = getLocalBounds();
    bypassDot.setBounds (r.removeFromRight (r.getHeight()).reduced (4));
    r.removeFromRight (juce::roundToInt (getHeight() * 2.6f));   // brand text space
    listenButton.setBounds (
        r.removeFromRight (juce::roundToInt (getHeight() * 2.2f)).reduced (0, 6));
    r.removeFromRight (8);
    presetBox.setBounds (
        r.removeFromRight (juce::roundToInt (getHeight() * 4.6f)).reduced (0, 6));
}

void HeaderBar::paint (juce::Graphics& g)
{
    const float h = (float) getHeight();

    g.setColour (porcelain::text);
    g.setFont (lnf.titleFont (h * 0.42f).withExtraKerningFactor (0.22f));
    g.drawText ("VOCAL SIBILANCE", getLocalBounds(), juce::Justification::centredLeft);

    g.setColour (porcelain::muted);
    g.setFont (lnf.labelFont (h * 0.26f).withExtraKerningFactor (0.25f));
    const auto brand = getLocalBounds()
                           .removeFromRight (getHeight() + juce::roundToInt (h * 2.6f))
                           .withTrimmedRight (getHeight() + 4);
    g.drawText ("DARARARA", brand, juce::Justification::centredRight);
}
} // namespace vs
