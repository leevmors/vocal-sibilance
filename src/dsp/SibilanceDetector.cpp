#include "SibilanceDetector.h"
#include <algorithm>
#include <cmath>

namespace vs
{
float SibilanceDetector::coeffForMs (float ms, double sampleRate)
{
    return 1.0f - std::exp (-1.0f / (0.001f * ms * (float) sampleRate));
}

void SibilanceDetector::prepare (double sr)
{
    bandAtt = coeffForMs (1.0f,   sr);
    bandRel = coeffForMs (60.0f,  sr);
    fullAtt = coeffForMs (15.0f,  sr);
    fullRel = coeffForMs (250.0f, sr);
    openAtt = coeffForMs (0.5f,   sr);
    openRel = coeffForMs (50.0f,  sr);
    reset();
}

void SibilanceDetector::reset()
{
    bandEnv = fullEnv = openness = 0.0f;
}

void SibilanceDetector::follow (float& env, float target, float attack, float release)
{
    env += (target > env ? attack : release) * (target - env);
}

float SibilanceDetector::processSample (float bandAbs, float fullAbs)
{
    follow (bandEnv, bandAbs, bandAtt, bandRel);
    follow (fullEnv, fullAbs, fullAtt, fullRel);

    // floor tiny envelopes to zero: host-agnostic denormal protection during silence
    if (bandEnv < 1.0e-15f) bandEnv = 0.0f;
    if (fullEnv < 1.0e-15f) fullEnv = 0.0f;

    float raw = 0.0f;
    // Broadband onsets (plosives) briefly drive the ratio high because the band
    // envelope attacks ~15x faster than the full envelope. This is deliberate
    // (fast ess capture); the clamp + 0.5 ms openness smoothing bound the effect.
    if (fullEnv > silenceEnv)
    {
        const float ratio = bandEnv / fullEnv;
        raw = std::clamp ((ratio - ratioLow) / (ratioHigh - ratioLow), 0.0f, 1.0f);
        raw = raw * raw * (3.0f - 2.0f * raw);   // smoothstep knee
    }
    follow (openness, raw, openAtt, openRel);
    if (openness < 1.0e-15f) openness = 0.0f;
    return openness;
}
} // namespace vs
