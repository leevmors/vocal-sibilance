#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "PluginProcessor.h"

namespace
{
void setParam (VocalSibilanceProcessor& proc, const char* id, float plainValue)
{
    auto* p = proc.apvts.getParameter (id);
    REQUIRE (p != nullptr);
    p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}
} // namespace

TEST_CASE ("state save/restore round-trips parameter values", "[processor]")
{
    VocalSibilanceProcessor a;
    setParam (a, "smooth", 77.0f);
    setParam (a, "rangeLo", 3000.0f);
    setParam (a, "listen", 1.0f);

    juce::MemoryBlock blob;
    a.getStateInformation (blob);

    VocalSibilanceProcessor b;
    b.setStateInformation (blob.getData(), (int) blob.getSize());

    CHECK (b.apvts.getRawParameterValue ("smooth")->load() == Catch::Approx (77.0f).margin (0.01));
    CHECK (b.apvts.getRawParameterValue ("rangeLo")->load() == Catch::Approx (3000.0f).margin (1.0));
    CHECK (b.apvts.getRawParameterValue ("listen")->load() == 1.0f);
}

TEST_CASE ("corrupt state blobs are ignored without crashing", "[processor]")
{
    VocalSibilanceProcessor p;
    const char junk[] = "not a valid state";
    p.setStateInformation (junk, (int) sizeof (junk));
    CHECK (p.apvts.getRawParameterValue ("smooth")->load() == Catch::Approx (35.0f).margin (0.01));
}

TEST_CASE ("processBlock at neutral is bit-transparent", "[processor]")
{
    VocalSibilanceProcessor proc;
    setParam (proc, "smooth", 0.0f);
    setParam (proc, "grit", 0.0f);
    proc.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buf (2, 512), ref (2, 512);
    juce::Random rng (1234);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buf.setSample (ch, i, rng.nextFloat() - 0.5f);
    ref.makeCopyOf (buf);

    juce::MidiBuffer midi;
    proc.processBlock (buf, midi);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            REQUIRE (buf.getSample (ch, i) == ref.getSample (ch, i));
}

TEST_CASE ("mono and stereo layouts accepted, mismatch rejected", "[processor]")
{
    VocalSibilanceProcessor p;
    juce::AudioProcessor::BusesLayout mono, stereo, mismatch;
    mono.inputBuses.add (juce::AudioChannelSet::mono());
    mono.outputBuses.add (juce::AudioChannelSet::mono());
    stereo.inputBuses.add (juce::AudioChannelSet::stereo());
    stereo.outputBuses.add (juce::AudioChannelSet::stereo());
    mismatch.inputBuses.add (juce::AudioChannelSet::mono());
    mismatch.outputBuses.add (juce::AudioChannelSet::stereo());

    CHECK (p.setBusesLayout (mono));
    CHECK (p.setBusesLayout (stereo));
    CHECK (! p.setBusesLayout (mismatch));
}
