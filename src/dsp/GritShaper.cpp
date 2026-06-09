#include "GritShaper.h"
#include <cmath>

namespace vs
{
void GritShaper::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numChannels, 2,                       // 2 stages -> 4x
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true);                                         // max quality: linear phase
    oversampling->initProcessing ((size_t) maxBlockSize);

    dcBlockers.clear();
    for (int ch = 0; ch < numChannels; ++ch)
        dcBlockers.emplace_back (
            juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 20.0f));
    reset();
}

void GritShaper::reset()
{
    if (oversampling != nullptr)
        oversampling->reset();
    for (auto& f : dcBlockers)
        f.reset();
    wasActive = false;
}

void GritShaper::setGrit (float a)      { grit = juce::jlimit (0.0f, 1.0f, a); }
void GritShaper::setCharacter (float a) { character = juce::jlimit (0.0f, 1.0f, a); }

float GritShaper::getLatencySamples() const
{
    return oversampling != nullptr ? oversampling->getLatencyInSamples() : 0.0f;
}

float GritShaper::shape (float x, float drive) const
{
    const float sym  = std::tanh (drive * x);              // tape-ish rounding
    const float bent = x + 0.35f * x * std::abs (x);       // even-order content
    const float asym = std::tanh (drive * 1.3f * bent);    // edgier lobe
    return (sym + character * (asym - sym)) / drive;       // unity small-signal gain
}

void GritShaper::process (juce::AudioBuffer<float>& band, const float* env, int numSamples)
{
    if (grit < 1.0e-4f)
    {
        if (wasActive)        // leaving the active state: clear filter history so
            reset();          // the next engage starts clean (no stale ringing)
        return;
    }
    wasActive = true;

    juce::dsp::AudioBlock<float> block (band.getArrayOfWritePointers(),
                                        (size_t) band.getNumChannels(),
                                        (size_t) numSamples);
    auto osBlock = oversampling->processSamplesUp (block);
    jassert (osBlock.getNumSamples() == 4 * (size_t) numSamples);   // 2-stage = exactly 4x

    for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
    {
        float* d = osBlock.getChannelPointer (ch);
        for (size_t i = 0; i < osBlock.getNumSamples(); ++i)
        {
            const float wet = grit * env[i >> 2];          // 4x: base-rate index i/4
            if (wet <= 0.0f)
                continue;
            const float drive = 1.0f + 9.0f * wet;
            d[i] += wet * (shape (d[i], drive) - d[i]);
        }
    }

    oversampling->processSamplesDown (block);

    for (int ch = 0; ch < band.getNumChannels(); ++ch)     // kill DC from the asym curve
    {
        float* p = band.getWritePointer (ch);
        auto& f = dcBlockers[(size_t) ch];
        for (int i = 0; i < numSamples; ++i)
            p[i] = f.processSample (p[i]);
    }
}
} // namespace vs
