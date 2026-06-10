#pragma once
#include <array>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../PluginProcessor.h"
#include "../dsp/CrossoverBands.h"
#include "PorcelainLookAndFeel.h"

namespace vs
{
// Hero display: live input spectrum, ember tint on detected sibilance inside
// the range, gain-reduction readout, draggable range handles.
class SibilanceDisplay : public juce::Component, private juce::Timer
{
public:
    explicit SibilanceDisplay (VocalSibilanceProcessor& proc);
    ~SibilanceDisplay() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<float> plotArea() const;
    float freqToX (float hz) const;
    float xToFreq (float x) const;
    int handleAt (float x) const;        // 0 = lo, 1 = hi, -1 = none

    VocalSibilanceProcessor& processor;

    static constexpr int fftOrder = 11, fftSize = 1 << fftOrder;   // 2048
    static constexpr int numBars = 64;
    static constexpr float fMin = 80.0f, fMax = 20000.0f;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize,
        juce::dsp::WindowingFunction<float>::hann };
    std::array<float, fftSize> sampleRing {};
    int ringPos = 0;
    std::array<float, 2 * fftSize> fftData {};
    std::array<float, numBars> barLevels {};

    int draggingHandle = -1;
    int hoveredHandle = -1;
    juce::RangedAudioParameter* loParam = nullptr;
    juce::RangedAudioParameter* hiParam = nullptr;
};
} // namespace vs
