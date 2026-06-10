#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>
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

TEST_CASE ("smooth percent reaches the engine scaled, and reduces sibilance", "[processor]")
{
    // smooth stays at its 35% default: if the *0.01f conversion were missing,
    // the engine would clamp 35.0 to 1.0 and reduction would hit ~15 dB,
    // failing the upper bound below.
    VocalSibilanceProcessor proc;
    proc.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    double phase = 0.0;
    for (int b = 0; b < 30; ++b)            // ~320 ms of a steady 8 kHz "ess"
    {
        for (int i = 0; i < 512; ++i)
        {
            const float v = 0.4f * (float) std::sin (phase);
            phase += 2.0 * 3.141592653589793 * 8000.0 / 48000.0;
            buf.setSample (0, i, v);
            buf.setSample (1, i, v);
        }
        proc.processBlock (buf, midi);
    }
    const float gr = proc.getUiGainReductionDb();
    CHECK (gr > 3.0f);    // smooth is doing something...
    CHECK (gr < 8.0f);    // ...at 35%, not at a mis-scaled 100%
}

TEST_CASE ("listen parameter routes the band solo through the processor", "[processor]")
{
    VocalSibilanceProcessor proc;
    auto* listen = proc.apvts.getParameter ("listen");
    REQUIRE (listen != nullptr);
    listen->setValueNotifyingHost (1.0f);
    proc.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    double phase = 0.0, sumIn = 0.0, sumOut = 0.0;
    for (int b = 0; b < 16; ++b)            // 300 Hz tone: far below the band
    {
        for (int i = 0; i < 512; ++i)
        {
            const float v = 0.5f * (float) std::sin (phase);
            phase += 2.0 * 3.141592653589793 * 300.0 / 48000.0;
            buf.setSample (0, i, v);
            buf.setSample (1, i, v);
        }
        if (b >= 8)
            for (int i = 0; i < 512; ++i)
                sumIn += (double) buf.getSample (0, i) * buf.getSample (0, i);
        proc.processBlock (buf, midi);
        if (b >= 8)
            for (int i = 0; i < 512; ++i)
                sumOut += (double) buf.getSample (0, i) * buf.getSample (0, i);
    }
    CHECK (std::sqrt (sumOut) < std::sqrt (sumIn) * 0.1);   // soloed band is near-silent
}
