#pragma once
#include <juce_dsp/juce_dsp.h>

namespace vs
{
// Splits audio into low / sibilant band / high with 4th-order Linkwitz-Riley
// crossovers. recombine() of untouched bands is magnitude-flat (allpass phase).
class CrossoverBands
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void setCrossoverFrequencies (float loHz, float hiHz);

    void split (const juce::AudioBuffer<float>& input);
    void recombine (juce::AudioBuffer<float>& dest, int numSamples) const;

    juce::AudioBuffer<float>& getBand()             { return band; }
    const juce::AudioBuffer<float>& getLow()  const { return low; }
    const juce::AudioBuffer<float>& getHigh() const { return high; }

    float getLoHz() const { return loHz; }
    float getHiHz() const { return hiHz; }

    static constexpr float minHz = 2000.0f, maxHz = 16000.0f, minGapRatio = 1.4142135f;

private:
    juce::dsp::LinkwitzRileyFilter<float> lowSplit, highSplit, lowAllpass;
    juce::AudioBuffer<float> low, band, high;
    float loHz = 4000.0f, hiHz = 11000.0f;
    int channels = 0;
    double sampleRateHz = 48000.0;
};
} // namespace vs
