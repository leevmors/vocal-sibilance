#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>
#include <vector>
#include "dsp/GritShaper.h"

namespace
{
constexpr double pi = 3.141592653589793;
}

TEST_CASE ("shaper has unity small-signal gain at character 0", "[grit]")
{
    vs::GritShaper gs;
    gs.prepare (48000.0, 512, 1);
    gs.setCharacter (0.0f);
    CHECK (gs.shapeForTest (0.001f, 1.0f) == Catch::Approx (0.001f).epsilon (0.01));
    CHECK (gs.shapeForTest (0.001f, 10.0f) == Catch::Approx (0.001f).epsilon (0.05));
}

TEST_CASE ("grit at zero is a bit-exact passthrough", "[grit]")
{
    vs::GritShaper gs;
    gs.prepare (48000.0, 512, 1);
    gs.setGrit (0.0f);

    juce::AudioBuffer<float> buf (1, 512);
    std::vector<float> env (512, 1.0f);
    for (int i = 0; i < 512; ++i)
        buf.setSample (0, i, (float) std::sin (2.0 * pi * 5000.0 * i / 48000.0));
    juce::AudioBuffer<float> ref;
    ref.makeCopyOf (buf);

    gs.process (buf, env.data(), 512);

    for (int i = 0; i < 512; ++i)
        REQUIRE (buf.getSample (0, i) == ref.getSample (0, i));
}

TEST_CASE ("reported latency is near zero (minimum-phase IIR)", "[grit]")
{
    vs::GritShaper gs;
    gs.prepare (48000.0, 512, 1);
    CHECK (gs.getLatencySamples() < 4.0f);
}

TEST_CASE ("aliasing sits >= 40 dB below the loudest harmonic", "[grit]")
{
    constexpr double sr = 48000.0;
    constexpr int N = 1 << 14;

    vs::GritShaper gs;
    gs.prepare (sr, 512, 1);
    gs.setGrit (1.0f);
    gs.setCharacter (1.0f);

    std::vector<float> env (512, 1.0f);
    juce::AudioBuffer<float> buf (1, 512);
    std::vector<float> captured;
    double phase = 0.0;
    const double inc = 2.0 * pi * 9000.0 / sr;     // 9 kHz -> bin-exact at N=16384

    while ((int) captured.size() < N + 4096)        // 4096 warmup + N analysed
    {
        for (int i = 0; i < 512; ++i)
        {
            buf.setSample (0, i, 0.5f * (float) std::sin (phase));
            phase += inc;
        }
        gs.process (buf, env.data(), 512);
        for (int i = 0; i < 512; ++i)
            captured.push_back (buf.getSample (0, i));
    }

    std::vector<float> fft (2 * N, 0.0f);
    std::copy (captured.end() - N, captured.end(), fft.begin());
    juce::dsp::WindowingFunction<float> win ((size_t) N,
        juce::dsp::WindowingFunction<float>::hann);
    win.multiplyWithWindowingTable (fft.data(), (size_t) N);
    juce::dsp::FFT engine (14);
    engine.performFrequencyOnlyForwardTransform (fft.data());

    auto binOf = [&] (double f) { return (int) std::round (f * N / sr); };
    auto peakAround = [&] (int bin)
    {
        float m = 0.0f;
        for (int k = bin - 4; k <= bin + 4; ++k)
            m = std::max (m, fft[(size_t) k]);
        return m;
    };

    const float maxHarm = std::max (peakAround (binOf (9000.0)),
                                    peakAround (binOf (18000.0)));

    float maxSpur = 0.0f;
    for (int k = binOf (100.0); k < binOf (23000.0); ++k)
    {
        if (std::abs (k - binOf (9000.0)) <= 6 || std::abs (k - binOf (18000.0)) <= 6)
            continue;
        maxSpur = std::max (maxSpur, fft[(size_t) k]);
    }

    const float dbDown = juce::Decibels::gainToDecibels (maxSpur / maxHarm);
    INFO ("worst spur is " << dbDown << " dB below loudest harmonic");
    CHECK (dbDown < -40.0f);
}
