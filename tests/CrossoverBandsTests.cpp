#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include "dsp/CrossoverBands.h"

namespace
{
constexpr double pi = 3.141592653589793;

float steadyStateRmsRatio (float freqHz, double sr = 48000.0)
{
    vs::CrossoverBands xo;
    xo.prepare (sr, 512, 1);
    xo.setCrossoverFrequencies (4000.0f, 11000.0f);

    juce::AudioBuffer<float> in (1, 512), out (1, 512);
    double phase = 0.0;
    const double inc = 2.0 * pi * freqHz / sr;

    double sumIn = 0.0, sumOut = 0.0;
    const int warmupBlocks = 8, measureBlocks = 16;

    for (int b = 0; b < warmupBlocks + measureBlocks; ++b)
    {
        for (int i = 0; i < 512; ++i)
        {
            in.setSample (0, i, (float) std::sin (phase));
            phase += inc;
        }
        xo.split (in);
        out.clear();
        xo.recombine (out, 512);

        if (b >= warmupBlocks)
            for (int i = 0; i < 512; ++i)
            {
                sumIn  += (double) in.getSample (0, i) * in.getSample (0, i);
                sumOut += (double) out.getSample (0, i) * out.getSample (0, i);
            }
    }
    return (float) std::sqrt (sumOut / sumIn);
}
} // namespace

TEST_CASE ("LR4 split+recombine is magnitude-flat", "[crossover]")
{
    for (float f : { 100.0f, 1000.0f, 4000.0f, 8000.0f, 11000.0f, 16000.0f })
    {
        const float ratioDb = juce::Decibels::gainToDecibels (steadyStateRmsRatio (f));
        INFO ("freq " << f << " Hz, deviation " << ratioDb << " dB");
        CHECK (std::abs (ratioDb) < 0.02f);
    }
}

TEST_CASE ("crossover enforces a minimum half-octave gap", "[crossover]")
{
    vs::CrossoverBands xo;
    xo.prepare (48000.0, 64, 1);
    xo.setCrossoverFrequencies (8000.0f, 8000.0f);          // illegal: zero gap
    CHECK (xo.getHiHz() >= xo.getLoHz() * 1.41f);

    xo.setCrossoverFrequencies (16000.0f, 16000.0f);        // hi pinned at ceiling
    CHECK (xo.getHiHz() >= xo.getLoHz() * 1.41f);
}

TEST_CASE ("split/recombine stays flat with small blocks after a large prepare", "[crossover]")
{
    // prepare at 512 but process in 64-sample blocks: flatness must not depend
    // on maxBlockSize, only on the per-call sample count.
    vs::CrossoverBands xo;
    xo.prepare (48000.0, 512, 1);
    xo.setCrossoverFrequencies (4000.0f, 11000.0f);

    juce::AudioBuffer<float> in (1, 64), out (1, 64);
    double phase = 0.0;
    const double inc = 2.0 * pi * 1000.0 / 48000.0;
    double sumIn = 0.0, sumOut = 0.0;

    for (int b = 0; b < 192; ++b)   // 64 warmup + 128 measured blocks
    {
        for (int i = 0; i < 64; ++i)
        {
            in.setSample (0, i, (float) std::sin (phase));
            phase += inc;
        }
        xo.split (in);
        out.clear();
        xo.recombine (out, 64);
        if (b >= 64)
            for (int i = 0; i < 64; ++i)
            {
                sumIn  += (double) in.getSample (0, i) * in.getSample (0, i);
                sumOut += (double) out.getSample (0, i) * out.getSample (0, i);
            }
    }
    const float ratioDb = juce::Decibels::gainToDecibels ((float) std::sqrt (sumOut / sumIn));
    CHECK (std::abs (ratioDb) < 0.02f);
}
