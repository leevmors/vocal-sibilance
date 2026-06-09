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
    INFO ("latency = " << gs.getLatencySamples() << " samples");
    CHECK (gs.getLatencySamples() >= 0.0f);
    CHECK (gs.getLatencySamples() < 5.0f);   // measured ~3.82 at 48 kHz with 3-stage (8x) min-phase polyphase IIR
}

TEST_CASE ("aliasing sits >= 40 dB below the loudest harmonic across the band", "[grit]")
{
    constexpr int N = 1 << 14;

    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        for (double fNominal : { 6000.0, 9000.0, 11000.0, 12000.0, 14000.0, 16000.0 })
        {
            // snap to an exact FFT bin so Hann leakage stays local
            const int fundBin = (int) std::round (fNominal * N / sr);
            const double f0 = (double) fundBin * sr / N;

            vs::GritShaper gs;
            gs.prepare (sr, 512, 1);
            gs.setGrit (1.0f);
            gs.setCharacter (1.0f);

            std::vector<float> env (512, 1.0f);
            juce::AudioBuffer<float> buf (1, 512);
            std::vector<float> captured;
            double phase = 0.0;
            const double inc = 2.0 * pi * f0 / sr;

            while ((int) captured.size() < N + 4096)
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

            const double nyq = sr * 0.5;
            auto binOf = [&] (double f) { return (int) std::round (f * N / sr); };
            auto peakAround = [&] (int bin)
            {
                float m = 0.0f;
                for (int k = std::max (1, bin - 4); k <= std::min (N / 2 - 1, bin + 4); ++k)
                    m = std::max (m, fft[(size_t) k]);
                return m;
            };

            // True harmonics (k*f0 below Nyquist) are intended signal, never spurs.
            // Harmonics in the top 5% near Nyquist sit in the downsampler transition
            // band, so they are excluded from spur counting but not trusted as the
            // reference level either.
            std::vector<int> harmonicBins;
            float maxHarm = 0.0f;
            for (int k = 1; k <= 16; ++k)
            {
                const double fh = k * f0;
                if (fh < nyq)
                {
                    harmonicBins.push_back (binOf (fh));
                    if (fh < 0.95 * nyq)
                        maxHarm = std::max (maxHarm, peakAround (binOf (fh)));
                }
            }
            REQUIRE (maxHarm > 0.0f);

            float maxSpur = 0.0f;
            int   argmaxSpurBin = 0;
            for (int k = binOf (100.0); k < binOf (0.95 * nyq); ++k)
            {
                bool nearHarmonic = false;
                for (int hb : harmonicBins)
                    if (std::abs (k - hb) <= 6) { nearHarmonic = true; break; }
                if (! nearHarmonic && fft[(size_t) k] > maxSpur)
                {
                    maxSpur = fft[(size_t) k];
                    argmaxSpurBin = k;
                }
            }

            const float dbDown = juce::Decibels::gainToDecibels (maxSpur / maxHarm);
            const double argmaxSpurFreq = (double) argmaxSpurBin * sr / N;
            INFO ("sr " << sr << " f0 " << f0 << " spur floor " << dbDown
                  << " dB  argmaxSpur " << argmaxSpurFreq << " Hz (bin " << argmaxSpurBin << ")");
            CHECK (dbDown < -40.0f);
        }
    }
}
