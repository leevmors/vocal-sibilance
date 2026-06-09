#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

namespace vs
{
// Single-producer (audio thread) / single-consumer (UI timer) mono sample FIFO.
// Overflow drops samples (display-only data, never worth blocking for).
class SpectrumFifo
{
public:
    void push (const juce::AudioBuffer<float>& buffer)
    {
        const int numCh = buffer.getNumChannels();
        int start1, size1, start2, size2;
        fifo.prepareToWrite (buffer.getNumSamples(), start1, size1, start2, size2);
        writeRegion (buffer, 0,     start1, size1, numCh);
        writeRegion (buffer, size1, start2, size2, numCh);
        fifo.finishedWrite (size1 + size2);
    }

    int pull (float* dest, int maxValues)
    {
        int start1, size1, start2, size2;
        fifo.prepareToRead (maxValues, start1, size1, start2, size2);
        for (int i = 0; i < size1; ++i) dest[i]         = storage[(size_t) (start1 + i)];
        for (int i = 0; i < size2; ++i) dest[size1 + i] = storage[(size_t) (start2 + i)];
        fifo.finishedRead (size1 + size2);
        return size1 + size2;
    }

private:
    void writeRegion (const juce::AudioBuffer<float>& buffer,
                      int srcOffset, int start, int size, int numCh)
    {
        for (int i = 0; i < size; ++i)
        {
            float v = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
                v += buffer.getSample (ch, srcOffset + i);
            storage[(size_t) (start + i)] = v / (float) numCh;
        }
    }

    juce::AbstractFifo fifo { 1 << 14 };
    std::vector<float> storage = std::vector<float> (1 << 14, 0.0f);
};
} // namespace vs
