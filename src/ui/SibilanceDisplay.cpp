#include "SibilanceDisplay.h"
#include <cmath>

namespace vs
{
SibilanceDisplay::SibilanceDisplay (VocalSibilanceProcessor& proc) : processor (proc)
{
    loParam = processor.apvts.getParameter ("rangeLo");
    hiParam = processor.apvts.getParameter ("rangeHi");
    jassert (loParam != nullptr && hiParam != nullptr);
    setTitle ("Sibilance display");
    startTimerHz (30);
}

SibilanceDisplay::~SibilanceDisplay()
{
    stopTimer();
}

juce::Rectangle<float> SibilanceDisplay::plotArea() const
{
    return getLocalBounds().toFloat().reduced (10.0f, 8.0f);
}

float SibilanceDisplay::freqToX (float hz) const
{
    const auto a = plotArea();
    const float t = std::log (hz / fMin) / std::log (fMax / fMin);
    return a.getX() + juce::jlimit (0.0f, 1.0f, t) * a.getWidth();
}

float SibilanceDisplay::xToFreq (float x) const
{
    const auto a = plotArea();
    const float t = juce::jlimit (0.0f, 1.0f, (x - a.getX()) / a.getWidth());
    return fMin * std::pow (fMax / fMin, t);
}

void SibilanceDisplay::timerCallback()
{
    float chunk[1024];
    int got = 0;
    while ((got = processor.getSpectrumFifo().pull (chunk, 1024)) > 0)
        for (int i = 0; i < got; ++i)
        {
            sampleRing[(size_t) ringPos] = chunk[i];
            ringPos = (ringPos + 1) % fftSize;
        }

    for (int i = 0; i < fftSize; ++i)        // time-ordered frame
        fftData[(size_t) i] = sampleRing[(size_t) ((ringPos + i) % fftSize)];
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const double sr = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
    for (int b = 0; b < numBars; ++b)
    {
        const float f0 = fMin * std::pow (fMax / fMin, (float) b / numBars);
        const float f1 = fMin * std::pow (fMax / fMin, (float) (b + 1) / numBars);
        const int k0 = juce::jlimit (1, fftSize / 2 - 1, (int) (f0 * fftSize / sr));
        const int k1 = juce::jlimit (k0, fftSize / 2 - 1, (int) (f1 * fftSize / sr));
        float mag = 0.0f;
        for (int k = k0; k <= k1; ++k)
            mag = std::max (mag, fftData[(size_t) k]);

        const float db = juce::Decibels::gainToDecibels (mag / (float) fftSize, -72.0f);
        const float level = juce::jlimit (0.0f, 1.0f,
                                          juce::jmap (db, -72.0f, -6.0f, 0.0f, 1.0f));
        barLevels[(size_t) b] = std::max (level, barLevels[(size_t) b] * 0.82f);
    }
    repaint();
}

void SibilanceDisplay::paint (juce::Graphics& g)
{
    const auto full = getLocalBounds().toFloat();
    g.setColour (porcelain::inset);
    g.fillRoundedRectangle (full, 8.0f);
    g.setColour (porcelain::line);
    g.drawRoundedRectangle (full.reduced (0.5f), 8.0f, 1.0f);

    const auto a = plotArea();
    const float loHz = loParam->convertFrom0to1 (loParam->getValue());
    const float hiHz = hiParam->convertFrom0to1 (hiParam->getValue());
    const float loX = freqToX (loHz), hiX = freqToX (hiHz);
    const float openness = processor.getUiOpenness();
    const bool listening = processor.apvts.getRawParameterValue ("listen")->load() > 0.5f;

    g.setColour (porcelain::accent.withAlpha (0.07f));            // active band tint
    g.fillRect (juce::Rectangle<float> (loX, a.getY(), hiX - loX, a.getHeight()));

    const float bw = a.getWidth() / numBars;                      // spectrum bars
    for (int b = 0; b < numBars; ++b)
    {
        const float x = a.getX() + b * bw;
        const float fc = fMin * std::pow (fMax / fMin, (b + 0.5f) / numBars);
        const float h = barLevels[(size_t) b] * a.getHeight();
        const bool inRange = fc >= loHz && fc <= hiHz;

        g.setColour (inRange && openness > 0.05f
                         ? porcelain::accent.withAlpha (0.35f + 0.6f * openness)
                         : porcelain::barIdle);
        g.fillRect (x + 0.5f, a.getBottom() - h, bw - 1.0f, h);
    }

    const float gr = processor.getUiGainReductionDb();            // GR readout
    if (gr > 0.3f)
    {
        const float grW = (hiX - loX) * juce::jlimit (0.0f, 1.0f, gr / 15.0f);
        g.setColour (porcelain::accent.withAlpha (0.8f));
        g.fillRect (loX, a.getY(), grW, 3.0f);
        g.setColour (porcelain::muted);
        g.setFont (juce::Font (juce::FontOptions (11.0f)));
        g.drawText (juce::String (-gr, 1) + " dB",
                    juce::Rectangle<float> (a.getRight() - 70.0f, a.getY(), 64.0f, 14.0f),
                    juce::Justification::centredRight);
    }

    if (listening)                                                // dim outside range
    {
        g.setColour (porcelain::face.withAlpha (0.55f));
        g.fillRect (juce::Rectangle<float> (a.getX(), a.getY(), loX - a.getX(), a.getHeight()));
        g.fillRect (juce::Rectangle<float> (hiX, a.getY(), a.getRight() - hiX, a.getHeight()));
    }

    for (int handle = 0; handle < 2; ++handle)                    // range handles
    {
        const float hx = handle == 0 ? loX : hiX;
        const bool hot = draggingHandle == handle
                         || handleAt ((float) getMouseXYRelative().x) == handle;
        g.setColour (hot ? porcelain::accent : porcelain::text.withAlpha (0.55f));
        g.fillRect (hx - 0.75f, a.getY(), 1.5f, a.getHeight());
        if (hot)
        {
            const float hz = handle == 0 ? loHz : hiHz;
            g.setFont (juce::Font (juce::FontOptions (11.0f)));
            g.drawText (juce::String (hz / 1000.0f, 1) + " kHz",
                        juce::Rectangle<float> (hx - 32.0f, a.getY(), 64.0f, 14.0f),
                        juce::Justification::centred);
        }
    }
}

int SibilanceDisplay::handleAt (float x) const
{
    const float loX = freqToX (loParam->convertFrom0to1 (loParam->getValue()));
    const float hiX = freqToX (hiParam->convertFrom0to1 (hiParam->getValue()));
    const float dLo = std::abs (x - loX), dHi = std::abs (x - hiX);
    if (dLo <= 8.0f && dLo <= dHi) return 0;
    if (dHi <= 8.0f) return 1;
    return -1;
}

void SibilanceDisplay::mouseDown (const juce::MouseEvent& e)
{
    draggingHandle = handleAt (e.position.x);
    if (draggingHandle == 0) loParam->beginChangeGesture();
    if (draggingHandle == 1) hiParam->beginChangeGesture();
}

void SibilanceDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingHandle < 0)
        return;
    float hz = xToFreq (e.position.x);
    if (draggingHandle == 0)
    {
        const float hiHz = hiParam->convertFrom0to1 (hiParam->getValue());
        hz = juce::jlimit (CrossoverBands::minHz, hiHz / CrossoverBands::minGapRatio, hz);
        loParam->setValueNotifyingHost (loParam->convertTo0to1 (hz));
    }
    else
    {
        const float loHz = loParam->convertFrom0to1 (loParam->getValue());
        hz = juce::jlimit (loHz * CrossoverBands::minGapRatio, CrossoverBands::maxHz, hz);
        hiParam->setValueNotifyingHost (hiParam->convertTo0to1 (hz));
    }
    repaint();
}

void SibilanceDisplay::mouseUp (const juce::MouseEvent&)
{
    if (draggingHandle == 0) loParam->endChangeGesture();
    if (draggingHandle == 1) hiParam->endChangeGesture();
    draggingHandle = -1;
}

void SibilanceDisplay::mouseMove (const juce::MouseEvent&)
{
    repaint();   // hover feedback for the handles
}
} // namespace vs
