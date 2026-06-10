#include "CrossoverBands.h"

namespace vs
{
void CrossoverBands::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    channels = numChannels;
    const juce::dsp::ProcessSpec spec { sampleRate,
                                        (juce::uint32) maxBlockSize,
                                        (juce::uint32) numChannels };
    lowSplit.prepare (spec);
    highSplit.prepare (spec);
    lowAllpass.prepare (spec);
    lowAllpass.setType (juce::dsp::LinkwitzRileyFilterType::allpass);

    low.setSize (numChannels, maxBlockSize);
    band.setSize (numChannels, maxBlockSize);
    high.setSize (numChannels, maxBlockSize);

    setCrossoverFrequencies (loHz, hiHz);
    reset();
}

void CrossoverBands::reset()
{
    lowSplit.reset();
    highSplit.reset();
    lowAllpass.reset();
}

void CrossoverBands::setCrossoverFrequencies (float newLo, float newHi)
{
    const float liveMaxHz = juce::jmin (maxHz, (float) (sampleRateHz * 0.49));
    loHz = juce::jlimit (minHz, liveMaxHz, newLo);
    hiHz = juce::jlimit (minHz, liveMaxHz, newHi);
    if (hiHz < loHz * minGapRatio)
        hiHz = juce::jmin (liveMaxHz, loHz * minGapRatio);
    if (hiHz < loHz * minGapRatio)              // hi hit the ceiling: pull lo down
        loHz = hiHz / minGapRatio;

    lowSplit.setCutoffFrequency (loHz);
    highSplit.setCutoffFrequency (hiHz);
    lowAllpass.setCutoffFrequency (hiHz);
}

void CrossoverBands::split (const juce::AudioBuffer<float>& input)
{
    const int n = input.getNumSamples();
    for (int ch = 0; ch < channels; ++ch)
    {
        const float* src = input.getReadPointer (ch);
        float* lo = low.getWritePointer (ch);
        float* bd = band.getWritePointer (ch);
        float* hi = high.getWritePointer (ch);

        for (int i = 0; i < n; ++i)
        {
            float l = 0.0f, rest = 0.0f;
            lowSplit.processSample (ch, src[i], l, rest);
            l = lowAllpass.processSample (ch, l);   // phase-align with the hi split
            float b = 0.0f, h = 0.0f;
            highSplit.processSample (ch, rest, b, h);
            lo[i] = l; bd[i] = b; hi[i] = h;
        }
    }
}

void CrossoverBands::recombine (juce::AudioBuffer<float>& dest, int numSamples) const
{
    jassert (dest.getNumChannels() == channels); // must match prepare()
    for (int ch = 0; ch < dest.getNumChannels(); ++ch)
    {
        dest.copyFrom (ch, 0, low,  ch, 0, numSamples);
        dest.addFrom  (ch, 0, band, ch, 0, numSamples);
        dest.addFrom  (ch, 0, high, ch, 0, numSamples);
    }
}
} // namespace vs
