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
    GIT_TAG main)
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

target_sources(VocalSibilance PRIVATE
    src/PluginProcessor.cpp)

target_include_directories(VocalSibilance PRIVATE src)

target_compile_definitions(VocalSibilance PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)   # allowed: AGPLv3 build

target_link_libraries(VocalSibilance
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
