#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "dsp/SibilanceEngine.h"
#include "state/SpectrumFifo.h"

class VocalSibilanceProcessor : public juce::AudioProcessor
{
public:
    VocalSibilanceProcessor();
    ~VocalSibilanceProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                          { return true; }

    const juce::String getName() const override              { return "Vocal Sibilance"; }
    bool acceptsMidi() const override                        { return false; }
    bool producesMidi() const override                       { return false; }
    bool isMidiEffect() const override                       { return false; }
    double getTailLengthSeconds() const override             { return 0.0; }

    int getNumPrograms() override                            { return 1; }
    int getCurrentProgram() override                         { return 0; }
    void setCurrentProgram (int) override                    {}
    const juce::String getProgramName (int) override         { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // UI access
    vs::SpectrumFifo& getSpectrumFifo()     { return spectrumFifo; }
    float getUiOpenness() const             { return uiOpenness.load(); }
    float getUiGainReductionDb() const      { return uiGainReduction.load(); }

    juce::AudioProcessorValueTreeState apvts;

    static constexpr int stateVersion = 1;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    vs::SibilanceEngine engine;
    vs::SpectrumFifo spectrumFifo;
    std::atomic<float> uiOpenness { 0.0f }, uiGainReduction { 0.0f };

    std::atomic<float>* pSmooth    = nullptr;
    std::atomic<float>* pGrit      = nullptr;
    std::atomic<float>* pCharacter = nullptr;
    std::atomic<float>* pMix       = nullptr;
    std::atomic<float>* pOutput    = nullptr;
    std::atomic<float>* pRangeLo   = nullptr;
    std::atomic<float>* pRangeHi   = nullptr;
    std::atomic<float>* pListen    = nullptr;
    std::atomic<float>* pBypass    = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalSibilanceProcessor)
};
