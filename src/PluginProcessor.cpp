#include "PluginProcessor.h"

VocalSibilanceProcessor::VocalSibilanceProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

bool VocalSibilanceProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out)
        return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void VocalSibilanceProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    // JUCE effect model: the buffer already contains the input, so leaving it
    // untouched is a true passthrough. ignoreUnused only silences the warning.
    juce::ignoreUnused (buffer);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalSibilanceProcessor();
}
