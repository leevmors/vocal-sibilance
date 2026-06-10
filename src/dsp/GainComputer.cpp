#include "GainComputer.h"
#include "DspUtils.h"
#include <algorithm>
#include <cmath>

namespace vs
{

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
    // openness arrives pre-shaped by the detector; this second smoothstep
    // deliberately steepens the overall S-curve (soft knee).
    const float knee = openness * openness * (3.0f - 2.0f * openness);
    const float targetDb = -maxDepthDb * smoothAmount * knee;
    const float target = std::exp (targetDb * 0.11512925f);   // 10^(dB/20), exp form
    gain += (target < gain ? attCoeff : relCoeff) * (target - gain);
    return gain;
}
} // namespace vs
