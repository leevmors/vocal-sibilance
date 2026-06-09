#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include "dsp/SibilanceDetector.h"

namespace
{
constexpr double pi = 3.141592653589793;
}

TEST_CASE ("detector opens on sibilant-dominated signal", "[detector]")
{
    vs::SibilanceDetector det;
    det.prepare (48000.0);

    float open = 0.0f;
    for (int i = 0; i < 4800; ++i)   // 100 ms of "ess": band energy == full energy
    {
        const float a = 0.3f * std::abs ((float) std::sin (2.0 * pi * 7000.0 * i / 48000.0));
        open = det.processSample (a, a);
    }
    CHECK (open > 0.7f);
}

TEST_CASE ("detector stays closed on vowel-dominated signal", "[detector]")
{
    vs::SibilanceDetector det;
    det.prepare (48000.0);

    float open = 0.0f;
    for (int i = 0; i < 9600; ++i)   // 200 ms: band energy ~ zero, full energy high
    {
        const float a = std::abs ((float) std::sin (2.0 * pi * 300.0 * i / 48000.0));
        open = det.processSample (0.001f * a, 0.3f * a);
    }
    CHECK (open < 0.1f);
}

TEST_CASE ("detector closes again after sibilance ends", "[detector]")
{
    vs::SibilanceDetector det;
    det.prepare (48000.0);

    float open = 0.0f;
    for (int i = 0; i < 4800; ++i)            // 100 ms ess
    {
        const float a = 0.3f * std::abs ((float) std::sin (2.0 * pi * 7000.0 * i / 48000.0));
        open = det.processSample (a, a);
    }
    REQUIRE (open > 0.7f);
    for (int i = 0; i < 9600; ++i)            // then 200 ms vowel
    {
        const float a = std::abs ((float) std::sin (2.0 * pi * 300.0 * i / 48000.0));
        open = det.processSample (0.001f * a, 0.3f * a);
    }
    CHECK (open < 0.15f);
}

TEST_CASE ("silence produces zero openness", "[detector]")
{
    vs::SibilanceDetector det;
    det.prepare (48000.0);
    float open = 1.0f;
    for (int i = 0; i < 1000; ++i)
        open = det.processSample (0.0f, 0.0f);
    CHECK (open == 0.0f);
}
