#pragma once
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>

namespace vs
{
// Detector-gated morphing waveshaper for the sibilant band, 8x oversampled
// with minimum-phase polyphase IIR halfbands (no integer latency).
class GritShaper
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void setGrit (float amount01);
    void setCharacter (float amount01);
    float getLatencySamples() const;

    // env: one openness value per *base-rate* sample, shared by all channels
    void process (juce::AudioBuffer<float>& band, const float* env, int numSamples);

    float shapeForTest (float x, float drive) const { return shape (x, drive); }

private:
    float shape (float x, float drive) const;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    std::vector<juce::dsp::IIR::Filter<float>> dcBlockers;
    float grit = 0.0f, character = 0.3f;
    bool wasActive = false;
};
} // namespace vs
