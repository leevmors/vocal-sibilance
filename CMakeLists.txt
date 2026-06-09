cmake_minimum_required(VERSION 3.22)
project(VocalSibilance VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

FetchContent_Declare(clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG 51a9359315298de632cf44e9d7524940868441e6)
FetchContent_MakeAvailable(clap-juce-extensions)

juce_add_plugin(VocalSibilance
    COMPANY_NAME "dararara"
    BUNDLE_ID com.dararara.vocalsibilance
    PLUGIN_MANUFACTURER_CODE Drra
    PLUGIN_CODE Vsib
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Vocal Sibilance")

clap_juce_extensions_plugin(TARGET VocalSibilance
    CLAP_ID "com.dararara.vocalsibilance"
    CLAP_FEATURES audio-effect)

juce_add_binary_data(VSAssets SOURCES
    assets/fonts/InterRegular.ttf
    assets/fonts/InterSemiBold.ttf
    presets/Gentle.xml
    presets/Standard.xml
    presets/BrightVocal.xml
    presets/TameHarshMic.xml
    presets/GrittyPop.xml
    presets/MaximumCharacter.xml)

target_sources(VocalSibilance PRIVATE
    src/PluginProcessor.cpp
    src/dsp/CrossoverBands.cpp
    src/dsp/SibilanceDetector.cpp
    src/dsp/GainComputer.cpp
    src/dsp/GritShaper.cpp
    src/dsp/SibilanceEngine.cpp)

target_include_directories(VocalSibilance PRIVATE src)

target_compile_definitions(VocalSibilance PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0    # allowed: AGPLv3 build
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(VocalSibilance
    PRIVATE
        VSAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------- tests ----------------
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(Catch2)

enable_testing()

juce_add_console_app(dsp_tests PRODUCT_NAME "dsp_tests")

target_sources(dsp_tests PRIVATE
    tests/SanityTests.cpp
    tests/CrossoverBandsTests.cpp
    src/dsp/CrossoverBands.cpp
    tests/SibilanceDetectorTests.cpp
    src/dsp/SibilanceDetector.cpp
    tests/GainComputerTests.cpp
    src/dsp/GainComputer.cpp
    tests/GritShaperTests.cpp
    src/dsp/GritShaper.cpp
    tests/SibilanceEngineTests.cpp
    src/dsp/SibilanceEngine.cpp
    tests/ProcessorTests.cpp
    src/PluginProcessor.cpp)

target_include_directories(dsp_tests PRIVATE src)

target_compile_definitions(dsp_tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_compile_definitions(VocalSibilance PRIVATE VS_USE_GENERIC_EDITOR=1)  # removed in Task 13
target_compile_definitions(dsp_tests PRIVATE VS_USE_GENERIC_EDITOR=1)       # permanent

target_link_libraries(dsp_tests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(dsp_tests)
