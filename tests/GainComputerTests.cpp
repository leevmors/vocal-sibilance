#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "dsp/GainComputer.h"

TEST_CASE ("full openness at full smooth approaches max depth (-15 dB)", "[gain]")
{
    vs::GainComputer gc;
    gc.prepare (48000.0);
    gc.setSmoothAmount (1.0f);

    float g = 1.0f;
    for (int i = 0; i < 4800; ++i)      // 100 ms, plenty for the 1 ms attack
        g = gc.processSample (1.0f);

    CHECK (g == Catch::Approx (0.17783f).margin (0.005f));   // 10^(-15/20)
}

TEST_CASE ("zero smooth leaves gain at exactly unity", "[gain]")
{
    vs::GainComputer gc;
    gc.prepare (48000.0);
    gc.setSmoothAmount (0.0f);

    float g = 0.0f;
    for (int i = 0; i < 100; ++i)
        g = gc.processSample (1.0f);

    CHECK (g == 1.0f);
}

TEST_CASE ("gain recovers when openness closes", "[gain]")
{
    vs::GainComputer gc;
    gc.prepare (48000.0);
    gc.setSmoothAmount (1.0f);

    for (int i = 0; i < 4800; ++i) gc.processSample (1.0f);
    float g = 0.0f;
    for (int i = 0; i < 1440; ++i) g = gc.processSample (0.0f);   // exactly one 30 ms tau
    CHECK (g > 0.55f);    // a much faster (swapped) release would already be ~1.0
    CHECK (g < 0.85f);    // pins the 30 ms time constant
    for (int i = 0; i < 8160; ++i) g = gc.processSample (0.0f);   // 200 ms total
    CHECK (g > 0.95f);
}

TEST_CASE ("half smooth produces partial reduction", "[gain]")
{
    vs::GainComputer gc;
    gc.prepare (48000.0);
    gc.setSmoothAmount (0.5f);

    float g = 1.0f;
    for (int i = 0; i < 4800; ++i)
        g = gc.processSample (1.0f);

    CHECK (g < 0.9f);      // smoothAmount is actually applied...
    CHECK (g > 0.17783f);  // ...but not at full depth
}
