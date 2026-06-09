#include "PresetManager.h"
#include "BinaryData.h"

namespace vs
{
namespace
{
    constexpr const char* savedParamIds[] =
        { "smooth", "grit", "character", "mix", "output", "rangeLo", "rangeHi" };
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    struct Bin { const char* data; int size; };
    const Bin bins[] = {
        { BinaryData::Gentle_xml,           BinaryData::Gentle_xmlSize },
        { BinaryData::Standard_xml,         BinaryData::Standard_xmlSize },
        { BinaryData::BrightVocal_xml,      BinaryData::BrightVocal_xmlSize },
        { BinaryData::TameHarshMic_xml,     BinaryData::TameHarshMic_xmlSize },
        { BinaryData::GrittyPop_xml,        BinaryData::GrittyPop_xmlSize },
        { BinaryData::MaximumCharacter_xml, BinaryData::MaximumCharacter_xmlSize },
    };
    for (const auto& b : bins)
        if (auto xml = juce::XmlDocument::parse (juce::String::fromUTF8 (b.data, b.size)))
            factory.push_back (std::move (xml));
}

juce::StringArray PresetManager::getFactoryNames() const
{
    juce::StringArray names;
    for (const auto& xml : factory)
        names.add (xml->getStringAttribute ("name"));
    return names;
}

void PresetManager::applyFactory (int index)
{
    if (index >= 0 && index < (int) factory.size())
        applyXml (*factory[(size_t) index]);
}

void PresetManager::applyXml (const juce::XmlElement& xml)
{
    for (auto* e : xml.getChildWithTagNameIterator ("PARAM"))
        if (auto* param = apvts.getParameter (e->getStringAttribute ("id")))
            param->setValueNotifyingHost (
                param->convertTo0to1 ((float) e->getDoubleAttribute ("value")));
}

bool PresetManager::saveUserPreset (const juce::File& file) const
{
    juce::XmlElement xml ("Preset");
    xml.setAttribute ("name", file.getFileNameWithoutExtension());
    for (auto* id : savedParamIds)
        if (auto* param = apvts.getParameter (id))
        {
            auto* e = xml.createNewChildElement ("PARAM");
            e->setAttribute ("id", id);
            e->setAttribute ("value", param->convertFrom0to1 (param->getValue()));
        }
    return file.getParentDirectory().createDirectory() && xml.writeTo (file);
}

bool PresetManager::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("Preset"))
        return false;
    applyXml (*xml);
    return true;
}

juce::File PresetManager::userPresetDirectory()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("dararara")
                   .getChildFile ("Vocal Sibilance")
                   .getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}
} // namespace vs
