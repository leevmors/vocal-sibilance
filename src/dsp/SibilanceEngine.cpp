#include "SibilanceEngine.h"
#include <cmath>

namespace vs
{
void SibilanceEngine::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    crossover.prepare (sampleRate, maxBlockSize, numChannels);
    detector.prepare (sampleRate);
    gainComputer.prepare (sampleRate);
    gritShaper.prepare (sampleRate, maxBlockSize, numChannels);

    dryBuffer.setSize (numChannels, maxBlockSize);
    envBuffer.assign ((size_t) maxBlockSize, 0.0f);

    wetFade.reset (sampleRate, 0.05);
    mixSmoothed.reset (sampleRate, 0.05);
    outputGain.reset (sampleRate, 0.02);
    wetFade.setCurrentAndTargetValue (0.0f);
    mixSmoothed.setCurrentAndTargetValue (1.0f);
    outputGain.setCurrentAndTargetValue (1.0f);

    // Mod A: initialise crossover frequency smoothers (20 ms ramp)
    rangeLoSm.reset (sampleRate, 0.02);
    rangeHiSm.reset (sampleRate, 0.02);
    rangeLoSm.setCurrentAndTargetValue (params.rangeLoHz);
    rangeHiSm.setCurrentAndTargetValue (params.rangeHiHz);
    primed = false;

    reset();
}

void SibilanceEngine::reset()
{
    crossover.reset();
    detector.reset();
    gainComputer.reset();
    gritShaper.reset();
    metrics = {};
}

void SibilanceEngine::setParams (const EngineParams& p)
{
    params = p;
    gainComputer.setSmoothAmount (p.smooth);
    gritShaper.setGrit (p.grit);
    gritShaper.setCharacter (p.character);
    // Mod A: target the smoothers instead of stepping the crossover directly
    if (! primed)   // first params after prepare: snap, don't sweep (session load)
    {
        rangeLoSm.setCurrentAndTargetValue (p.rangeLoHz);
        rangeHiSm.setCurrentAndTargetValue (p.rangeHiHz);
        primed = true;
    }
    else
    {
        rangeLoSm.setTargetValue (p.rangeLoHz);
        rangeHiSm.setTargetValue (p.rangeHiHz);
    }
}

void SibilanceEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels();
    const int n = buffer.getNumSamples();

    for (int ch = 0; ch < numCh; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, n);

    // Mod A: advance smoothers by the block and apply once per block
    const float loNow = rangeLoSm.skip (n);
    const float hiNow = rangeHiSm.skip (n);
    crossover.setCrossoverFrequencies (loNow, hiNow);

    crossover.split (buffer);
    auto& band = crossover.getBand();

    float minGain = 1.0f;
    float opennessSum = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        float bandAbs = 0.0f, fullAbs = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)            // linked stereo detection
        {
            bandAbs = std::max (bandAbs, std::abs (band.getSample (ch, i)));
            fullAbs = std::max (fullAbs, std::abs (dryBuffer.getSample (ch, i)));
        }
        const float open = detector.processSample (bandAbs, fullAbs);
        envBuffer[(size_t) i] = open;
        opennessSum += open;

        const float g = gainComputer.processSample (open);
        minGain = std::min (minGain, g);
        for (int ch = 0; ch < numCh; ++ch)
            band.setSample (ch, i, band.getSample (ch, i) * g);
    }

    // Mod B: note on grit-band phase from the oversampling round-trip
    // The grit stage's oversampling round-trip applies a small minimum-phase
    // shift to the band even when the detector is closed (wet == 0). The
    // shift is sub-sample; recombine flatness with grit engaged is covered
    // by the "grit on a non-sibilant vowel" test.
    gritShaper.process (band, envBuffer.data(), n);

    if (params.listen && ! params.bypassed)
    {
        for (int ch = 0; ch < numCh; ++ch)
            buffer.copyFrom (ch, 0, band, ch, 0, n);
    }
    else
    {
        crossover.recombine (buffer, n);

        const bool active = ! params.bypassed
                            && (params.smooth > 1.0e-4f || params.grit > 1.0e-4f);
        wetFade.setTargetValue (active ? 1.0f : 0.0f);
        mixSmoothed.setTargetValue (params.mix);

        // w==0 gives bit-exact dry ONLY while the wet path is finite; a non-finite
        // p would turn 0*(p-d) into NaN. The output scrub below backstops that.
        for (int i = 0; i < n; ++i)
        {
            const float w = wetFade.getNextValue() * mixSmoothed.getNextValue();
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float d = dryBuffer.getSample (ch, i);
                const float p = buffer.getSample (ch, i);
                buffer.setSample (ch, i, d + w * (p - d));   // w==0 -> bit-exact dry
            }
        }
    }

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (params.outputDb));
    for (int i = 0; i < n; ++i)
    {
        const float g = outputGain.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
        {
            float v = buffer.getSample (ch, i) * g;
            if (! std::isfinite (v))                  // NaN/Inf scrub: never spray garbage
                v = 0.0f;
            buffer.setSample (ch, i, v);
        }
    }

    metrics.openness = n > 0 ? opennessSum / (float) n : 0.0f;
    metrics.gainReductionDb = -juce::Decibels::gainToDecibels (minGain);
}
} // namespace vs
