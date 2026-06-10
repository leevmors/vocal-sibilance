#pragma once

namespace vs
{
// Maps detector openness + the user's Smooth amount onto a smoothed linear
// gain (1.0 down to ~0.178 = -15 dB) for the sibilant band.
class GainComputer
{
public:
    static constexpr float maxDepthDb = 15.0f;

    void prepare (double sampleRate);
    void reset();
    void setSmoothAmount (float amount01);
    float processSample (float openness);   // returns linear band gain

private:
    float smoothAmount = 0.35f;
    float gain = 1.0f;
    float attCoeff = 0.0f, relCoeff = 0.0f;
};
} // namespace vs
