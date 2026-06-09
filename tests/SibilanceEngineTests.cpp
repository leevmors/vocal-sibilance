#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>
#include "dsp/SibilanceEngine.h"

namespace
{
constexpr double pi = 3.141592653589793;

vs::EngineParams neutralParams()
{
    vs::EngineParams p;
    p.smooth = 0.0f; p.grit = 0.0f; p.character = 0.3f;
    p.mix = 1.0f; p.outputDb = 0.0f;
    p.rangeLoHz = 4000.0f; p.rangeHiHz = 11000.0f;
    p.listen = false; p.bypassed = false;
    return p;
}

void fillSine (juce::AudioBuffer<float>& b, double freq, double sr, double& phase, float amp)
{
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const float v = amp * (float) std::sin (phase);
        phase += 2.0 * pi * freq / sr;
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            b.setSample (ch, i, v);
    }
}

double rmsOf (const std::vector<float>& v, size_t from, size_t to)
{
    double s = 0.0;
    for (size_t i = from; i < to; ++i)
        s += (double) v[i] * v[i];
    return std::sqrt (s / (double) (to - from));
}
} // namespace

TEST_CASE ("neutral settings are bit-transparent", "[engine]")
{
    vs::SibilanceEngine eng;
    eng.prepare (48000.0, 512, 2);
    eng.setParams (neutralParams());

    juce::Random rng (42);
    for (int block = 0; block < 8; ++block)
    {
        juce::AudioBuffer<float> buf (2, 512), ref (2, 512);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                buf.setSample (ch, i, rng.nextFloat() * 1.6f - 0.8f);
        ref.makeCopyOf (buf);

        eng.process (buf);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                REQUIRE (buf.getSample (ch, i) == ref.getSample (ch, i));
    }
}

TEST_CASE ("engine reduces a sibilant burst but not the vowel", "[engine]")
{
    constexpr double sr = 48000.0;
    vs::SibilanceEngine eng;
    eng.prepare (sr, 512, 1);
    auto p = neutralParams();
    p.smooth = 1.0f;
    eng.setParams (p);

    const int vowelN = 14400, essN = 7200;   // 300 ms @300 Hz, then 150 ms @8 kHz
    std::vector<float> in, out;
    double phase = 0.0;
    juce::AudioBuffer<float> buf (1, 512);

    auto run = [&] (double freq, float amp, int total)
    {
        int done = 0;
        while (done < total)
        {
            const int n = std::min (512, total - done);
            buf.setSize (1, n, false, false, true);
            fillSine (buf, freq, sr, phase, amp);
            for (int i = 0; i < n; ++i) in.push_back (buf.getSample (0, i));
            eng.process (buf);
            for (int i = 0; i < n; ++i) out.push_back (buf.getSample (0, i));
            done += n;
        }
    };
    run (300.0, 0.5f, vowelN);
    run (8000.0, 0.4f, essN);

    // vowel: steady tail (skip first 100 ms = crossfade + filter settle)
    const double vowelInDb  = 20.0 * std::log10 (rmsOf (in,  4800, (size_t) vowelN));
    const double vowelOutDb = 20.0 * std::log10 (rmsOf (out, 4800, (size_t) vowelN));
    CHECK (std::abs (vowelOutDb - vowelInDb) < 0.25);

    // ess: skip 20 ms attack, then expect >= 6 dB average reduction
    const size_t essFrom = (size_t) vowelN + 960, essTo = (size_t) (vowelN + essN);
    const double essInDb  = 20.0 * std::log10 (rmsOf (in,  essFrom, essTo));
    const double essOutDb = 20.0 * std::log10 (rmsOf (out, essFrom, essTo));
    CHECK (essInDb - essOutDb > 6.0);

    CHECK (eng.getMetrics().gainReductionDb > 6.0f);
}

TEST_CASE ("listen solos the band", "[engine]")
{
    // a 300 Hz tone is far below the band: listen output must be near-silent
    constexpr double sr = 48000.0;
    vs::SibilanceEngine eng;
    eng.prepare (sr, 512, 1);
    auto p = neutralParams();
    p.listen = true;
    eng.setParams (p);

    juce::AudioBuffer<float> buf (1, 512);
    double phase = 0.0, sumIn = 0.0, sumOut = 0.0;
    for (int b = 0; b < 16; ++b)
    {
        fillSine (buf, 300.0, sr, phase, 0.5f);
        if (b >= 8)
            for (int i = 0; i < 512; ++i)
                sumIn += (double) buf.getSample (0, i) * buf.getSample (0, i);
        eng.process (buf);
        if (b >= 8)
            for (int i = 0; i < 512; ++i)
                sumOut += (double) buf.getSample (0, i) * buf.getSample (0, i);
    }
    CHECK (std::sqrt (sumOut) < std::sqrt (sumIn) * 0.1);   // > 20 dB down
}

TEST_CASE ("bypass is bit-transparent even with smooth up", "[engine]")
{
    vs::SibilanceEngine eng;
    eng.prepare (48000.0, 512, 2);
    auto p = neutralParams();
    p.smooth = 1.0f;
    p.bypassed = true;
    eng.setParams (p);

    juce::Random rng (7);
    juce::AudioBuffer<float> buf (2, 512), ref (2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buf.setSample (ch, i, rng.nextFloat() - 0.5f);
    ref.makeCopyOf (buf);

    eng.process (buf);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            REQUIRE (buf.getSample (ch, i) == ref.getSample (ch, i));
}

TEST_CASE ("grit on a non-sibilant vowel adds nothing audible", "[engine]")
{
    // 300 Hz vowel: openness ~ 0, so detector-gated grit must leave level
    // intact even at full Grit (the OS round-trip phase shift is inaudible).
    constexpr double sr = 48000.0;
    vs::SibilanceEngine eng;
    eng.prepare (sr, 512, 1);
    auto p = neutralParams();
    p.grit = 1.0f;
    p.character = 1.0f;
    eng.setParams (p);

    juce::AudioBuffer<float> buf (1, 512);
    double phase = 0.0, sumIn = 0.0, sumOut = 0.0;
    for (int b = 0; b < 32; ++b)        // ~340 ms; fades settled after 8 blocks
    {
        fillSine (buf, 300.0, sr, phase, 0.5f);
        if (b >= 16)
            for (int i = 0; i < 512; ++i)
                sumIn += (double) buf.getSample (0, i) * buf.getSample (0, i);
        eng.process (buf);
        if (b >= 16)
            for (int i = 0; i < 512; ++i)
                sumOut += (double) buf.getSample (0, i) * buf.getSample (0, i);
    }
    const double dbDiff = 20.0 * std::log10 (std::sqrt (sumOut / sumIn));
    CHECK (std::abs (dbDiff) < 0.05);
}

TEST_CASE ("bit-exact neutrality is restored after automation returns to zero", "[engine]")
{
    vs::SibilanceEngine eng;
    eng.prepare (48000.0, 512, 2);
    auto p = neutralParams();

    juce::Random rng (99);
    juce::AudioBuffer<float> buf (2, 512), ref (2, 512);
    auto fillNoise = [&]
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                buf.setSample (ch, i, rng.nextFloat() - 0.5f);
    };

    p.smooth = 1.0f;                       // engage...
    eng.setParams (p);
    for (int b = 0; b < 8; ++b) { fillNoise(); eng.process (buf); }

    p.smooth = 0.0f;                       // ...then return to neutral
    eng.setParams (p);
    for (int b = 0; b < 8; ++b) { fillNoise(); eng.process (buf); }   // 85 ms >> 50 ms fade

    fillNoise();
    ref.makeCopyOf (buf);
    eng.process (buf);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            REQUIRE (buf.getSample (ch, i) == ref.getSample (ch, i));
}
