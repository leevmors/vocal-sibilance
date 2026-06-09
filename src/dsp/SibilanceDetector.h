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
    static void follow (float& env, float target, float attack, float release);

    float bandEnv = 0.0f, fullEnv = 0.0f, openness = 0.0f;
    float bandAtt = 0.0f, bandRel = 0.0f, fullAtt = 0.0f, fullRel = 0.0f, openAtt = 0.0f, openRel = 0.0f;

    static constexpr float ratioLow   = 0.30f;   // openness 0 at/below this band/full ratio
    static constexpr float ratioHigh  = 0.85f;   // openness 1 at/above
    static constexpr float silenceEnv = 1.0e-3f; // ~ -60 dBFS: below this, stay closed
};
} // namespace vs
