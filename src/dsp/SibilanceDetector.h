#pragma once

namespace vs
{
// Converts per-sample band/full magnitudes into 0..1 "sibilance openness".
// Threshold-free: a fast band envelope is compared against a slower
// full-signal envelope, so behaviour tracks the singer's level automatically.
class SibilanceDetector
{
public:
    void prepare (double sampleRate);
    void reset();
    float processSample (float bandAbs, float fullAbs);   // returns openness 0..1

private:
    static float coeffForMs (float ms, double sampleRate);
    static void follow (float& env, float target, float attack, float release);

    float bandEnv = 0, fullEnv = 0, openness = 0;
    float bandAtt = 0, bandRel = 0, fullAtt = 0, fullRel = 0, openAtt = 0, openRel = 0;

    static constexpr float ratioLow   = 0.30f;   // openness 0 at/below this band/full ratio
    static constexpr float ratioHigh  = 0.85f;   // openness 1 at/above
    static constexpr float silenceEnv = 1.0e-3f; // ~ -60 dBFS: below this, stay closed
};
} // namespace vs
