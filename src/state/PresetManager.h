#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>

namespace vs
{
// Factory presets from BinaryData XML + user preset save/load on disk.
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& state);

    juce::StringArray getFactoryNames() const;
    void applyFactory (int index);                        // 0-based
    bool saveUserPreset (const juce::File& file) const;   // .vsib (XML inside)
    bool loadUserPreset (const juce::File& file);
    static juce::File userPresetDirectory();

private:
    void applyXml (const juce::XmlElement& xml);

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<std::unique_ptr<juce::XmlElement>> factory;
};
} // namespace vs
