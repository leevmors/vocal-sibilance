#include <catch2/catch_test_macros.hpp>
#include <juce_dsp/juce_dsp.h>

TEST_CASE ("test infrastructure compiles and links JUCE dsp", "[sanity]")
{
    juce::SmoothedValue<float> sv;
    sv.setCurrentAndTargetValue (1.0f);
    REQUIRE (sv.getNextValue() == 1.0f);
}
