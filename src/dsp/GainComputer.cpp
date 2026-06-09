#include "GainComputer.h"
#include <algorithm>
#include <cmath>

namespace vs
{
namespace
{
    float coeffForMs (float ms, double sampleRate)
    {
        return 1.0f - std::exp (-1.0f / (0.001f * ms * (float) sampleRate));
    }
} // namespace

void GainComputer::prepare (double sr)
{
    attCoeff = coeffForMs (1.0f,  sr);   // clamp down fast
    relCoeff = coeffForMs (30.0f, sr);   // recover gently
    reset();
}

void GainComputer::reset()
{
    gain = 1.0f;
}

void GainComputer::setSmoothAmount (float a)
{
    smoothAmount = std::clamp (a, 0.0f, 1.0f);
}

float GainComputer::processSample (float openness)
{
    const float knee = openness * openness * (3.0f - 2.0f * openness);  // soft knee
    const float targetDb = -maxDepthDb * smoothAmount * knee;
    const float target = std::pow (10.0f, targetDb * (1.0f / 20.0f));
    gain += (target < gain ? attCoeff : relCoeff) * (target - gain);
    return gain;
}
} // namespace vs
