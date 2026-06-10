#pragma once
#include <cmath>

namespace vs
{
// One-pole coefficient for a given time constant in milliseconds.
inline float coeffForMs (float ms, double sampleRate)
{
    return 1.0f - std::exp (-1.0f / (0.001f * ms * (float) sampleRate));
}
} // namespace vs
