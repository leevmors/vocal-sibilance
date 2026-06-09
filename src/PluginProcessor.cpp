#include "PluginProcessor.h"
#if ! VS_USE_GENERIC_EDITOR
 #include "ui/PluginEditor.h"
#endif

juce::AudioProcessorValueTreeState::ParameterLayout
VocalSibilanceProcessor::createParameterLayout()
{
    using FloatParam = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = [] (const char* id, const char* name, float defaultValue)
    {
        return std::make_unique<FloatParam> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), defaultValue,
            juce::AudioParameterFloatAttributes().withLabel ("%"));
    };

    layout.add (pct ("smooth",    "Smooth",    35.0f));
    layout.add (pct ("grit",      "Grit",       0.0f));
    layout.add (pct ("character", "Character", 30.0f));
    layout.add (pct ("mix",       "Mix",      100.0f));
    layout.add (std::make_unique<FloatParam> (
        juce::ParameterID { "output", 1 }, "Output",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    const juce::NormalisableRange<float> freqRange (2000.0f, 16000.0f, 1.0f, 0.5f);
    layout.add (std::make_unique<FloatParam> (
        juce::ParameterID { "rangeLo", 1 }, "Range Low", freqRange, 4000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    layout.add (std::make_unique<FloatParam> (
        juce::ParameterID { "rangeHi", 1 }, "Range High", freqRange, 11000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "listen", 1 }, "Listen", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return layout;
}

VocalSibilanceProcessor::VocalSibilanceProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    pSmooth    = apvts.getRawParameterValue ("smooth");
    pGrit      = apvts.getRawParameterValue ("grit");
    pCharacter = apvts.getRawParameterValue ("character");
    pMix       = apvts.getRawParameterValue ("mix");
    pOutput    = apvts.getRawParameterValue ("output");
    pRangeLo   = apvts.getRawParameterValue ("rangeLo");
    pRangeHi   = apvts.getRawParameterValue ("rangeHi");
    pListen    = apvts.getRawParameterValue ("listen");
    pBypass    = apvts.getRawParameterValue ("bypass");
}

bool VocalSibilanceProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out)
        return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void VocalSibilanceProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock,
                    juce::jmax (1, getTotalNumInputChannels()));
    setLatencySamples (engine.getLatencySamples());
}

void VocalSibilanceProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    spectrumFifo.push (buffer);          // pre-processing input for the display

    vs::EngineParams p;
    p.smooth     = pSmooth->load()    * 0.01f;
    p.grit       = pGrit->load()      * 0.01f;
    p.character  = pCharacter->load() * 0.01f;
    p.mix        = pMix->load()       * 0.01f;
    p.outputDb   = pOutput->load();
    p.rangeLoHz  = pRangeLo->load();
    p.rangeHiHz  = pRangeHi->load();
    p.listen     = pListen->load() > 0.5f;
    p.bypassed   = pBypass->load() > 0.5f;

    engine.setParams (p);
    engine.process (buffer);

    const auto m = engine.getMetrics();
    uiOpenness.store (m.openness);
    uiGainReduction.store (m.gainReductionDb);
}

juce::AudioProcessorEditor* VocalSibilanceProcessor::createEditor()
{
#if VS_USE_GENERIC_EDITOR
    return new juce::GenericAudioProcessorEditor (*this);
#else
    return new VocalSibilanceEditor (*this);
#endif
}

void VocalSibilanceProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("stateVersion", stateVersion, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalSibilanceProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            // v1 is the only schema so far; branch on this when a future
            // version changes parameter semantics.
            const int loadedVersion = (int) tree.getProperty ("stateVersion", 1);
            juce::ignoreUnused (loadedVersion);
            apvts.replaceState (tree);
        }
    }
    // unknown/corrupt blobs and missing fields: keep current/default values
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalSibilanceProcessor();
}
