#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include "CrossoverBands.h"
#include "SibilanceDetector.h"
#include "GainComputer.h"
#include "GritShaper.h"

namespace vs
{
struct EngineParams
{
    float smooth = 0.35f;        // 0..1
    float grit = 0.0f;           // 0..1
    float character = 0.3f;      // 0..1
    float mix = 1.0f;            // 0..1
    float outputDb = 0.0f;       // -12..+12
    float rangeLoHz = 4000.0f;
    float rangeHiHz = 11000.0f;
    bool listen = false;
    bool bypassed = false;
};

struct EngineMetrics
{
    float openness = 0.0f;          // block-average detector openness 0..1
    float gainReductionDb = 0.0f;   // peak reduction this block (positive dB)
};

class SibilanceEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void setParams (const EngineParams& p);   // once per block, from cached atomics
    void process (juce::AudioBuffer<float>& buffer);

    EngineMetrics getMetrics() const { return metrics; }
    int getLatencySamples() const    { return 0; }

private:
    CrossoverBands crossover;
    SibilanceDetector detector;
    GainComputer gainComputer;
    GritShaper gritShaper;

    juce::AudioBuffer<float> dryBuffer;
    std::vector<float> envBuffer;

    juce::SmoothedValue<float> wetFade;       // 0 -> bit-exact dry (neutral/bypass)
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> outputGain;

    // Mod A: smoothed crossover frequencies to prevent zipper noise
    juce::SmoothedValue<float> rangeLoSm, rangeHiSm;

    EngineParams params;
    bool primed = false;
    EngineMetrics metrics;
};
} // namespace vs
