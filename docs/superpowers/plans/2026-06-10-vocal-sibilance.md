# Vocal Sibilance Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build Vocal Sibilance v1.0 — a free, open-source (AGPLv3) de-esser plugin (VST3/AU/CLAP, Windows+macOS) with detector-gated grit saturation and a Porcelain Display-First UI.

**Architecture:** JUCE 8 (C++20, CMake) plugin. A pure-DSP engine (`src/dsp/`, no GUI deps, Catch2-tested) does Linkwitz-Riley band isolation → relative sibilance detection → soft-knee gain reduction → 8× oversampled morphing waveshaper. `PluginProcessor` exposes 9 APVTS parameters and feeds the UI via a lock-free FIFO + atomics. The UI is fully vector-drawn (custom LookAndFeel).

**Tech Stack:** JUCE 8.0.4 (FetchContent), clap-juce-extensions, Catch2 v3.5.2, CMake ≥3.22, MSVC 2022 (Win) / Xcode clang (mac CI), GitHub Actions, pluginval, Inno Setup, Inter font (OFL).

**Spec:** `docs/superpowers/specs/2026-06-10-vocal-sibilance-design.md` — read it first. The spec is the contract; this plan is the route.

**Repo root (working dir for all commands):** `C:\Users\leeviosomars\vocal-sibilance`. Shell commands below are PowerShell unless noted. Build outputs land in `build/`.

---

## File Structure (what exists when we're done)

| Path | Responsibility |
|---|---|
| `CMakeLists.txt` | Whole build: JUCE/Catch2/clap fetch, plugin target, assets, tests |
| `src/dsp/CrossoverBands.{h,cpp}` | LR4 3-band split + flat recombine |
| `src/dsp/SibilanceDetector.{h,cpp}` | Threshold-free 0..1 "openness" detector |
| `src/dsp/GainComputer.{h,cpp}` | Openness+Smooth → smoothed linear band gain |
| `src/dsp/GritShaper.{h,cpp}` | 8× oversampled morphing waveshaper, detector-gated |
| `src/dsp/SibilanceEngine.{h,cpp}` | Wires the chain; mix/output/listen/bypass; metrics |
| `src/state/SpectrumFifo.h` | Lock-free audio→UI sample FIFO (header-only) |
| `src/state/PresetManager.{h,cpp}` | Factory (BinaryData XML) + user preset I/O |
| `src/PluginProcessor.{h,cpp}` | APVTS params, state save/restore, engine hookup |
| `src/ui/PorcelainLookAndFeel.{h,cpp}` | Colors, fonts, knob/button/combo drawing |
| `src/ui/PorcelainKnob.h` | Slider subclass with emphasis flag (header-only) |
| `src/ui/SibilanceDisplay.{h,cpp}` | Spectrum, ember detection tint, GR overlay, range handles |
| `src/ui/HeaderBar.{h,cpp}` | Wordmark, preset combo, LISTEN pill, brand, bypass dot |
| `src/ui/PluginEditor.{h,cpp}` | Assembles UI, resizable 60–200 %, scale persistence |
| `tests/SanityTests.cpp` … `tests/ProcessorTests.cpp` | Catch2 suites (one file per DSP unit) |
| `presets/*.xml` | 6 factory presets (embedded via BinaryData) |
| `assets/fonts/InterRegular.ttf`, `InterSemiBold.ttf` | Bundled UI font |
| `.github/workflows/build.yml`, `release.yml` | CI build/test/pluginval; tagged releases |
| `packaging/windows-installer.iss`, `packaging/make-macos-pkg.sh` | Installers |
| `README.md`, `LICENSE` | Docs + AGPLv3 |

Naming conventions used everywhere: namespace `vs` for engine/state code; parameter IDs are `smooth, grit, character, mix, output, rangeLo, rangeHi, listen, bypass`; class names `VocalSibilanceProcessor` / `VocalSibilanceEditor`.

---

### Task 0: Toolchain setup (Windows dev machine)

The machine has git but **no CMake and no MSVC**. Nothing to commit in this task.

**Files:** none.

- [ ] **Step 0.1: Install CMake and VS 2022 Build Tools**

```powershell
winget install --id Kitware.CMake -e --accept-source-agreements --accept-package-agreements
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

The Build Tools install is large (~7 GB) and can take 10–30 minutes. `--wait` makes winget block until done.

- [ ] **Step 0.2: Verify (in a NEW shell, so PATH updates apply)**

Run: `cmake --version`
Expected: `cmake version 3.2x` (≥ 3.22)

Run: `Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC"`
Expected: `True`

If winget is unavailable, download installers manually: cmake.org/download and visualstudio.microsoft.com/downloads (Build Tools for Visual Studio 2022 → "Desktop development with C++").

---

### Task 1: Project scaffold — passthrough plugin that builds

A minimal passthrough plugin proving the whole toolchain (JUCE fetch, VST3+CLAP+Standalone targets) before any real code. The processor here is a stub; Task 8 replaces it with the real one.

**Files:**
- Create: `CMakeLists.txt`
- Create: `src/PluginProcessor.h`
- Create: `src/PluginProcessor.cpp`

- [ ] **Step 1.1: Write `CMakeLists.txt`**

```cmake
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
```

(AU is listed but JUCE only builds it on macOS — Windows builds skip it automatically.)

- [ ] **Step 1.2: Write stub `src/PluginProcessor.h`**

```cpp
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

class VocalSibilanceProcessor : public juce::AudioProcessor
{
public:
    VocalSibilanceProcessor();
    ~VocalSibilanceProcessor() override = default;

    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor (*this);
    }
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

    void getStateInformation (juce::MemoryBlock&) override   {}
    void setStateInformation (const void*, int) override     {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalSibilanceProcessor)
};
```

- [ ] **Step 1.3: Write stub `src/PluginProcessor.cpp`**

```cpp
#include "PluginProcessor.h"

VocalSibilanceProcessor::VocalSibilanceProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

bool VocalSibilanceProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out)
        return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void VocalSibilanceProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    // passthrough for now
    juce::ignoreUnused (buffer);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalSibilanceProcessor();
}
```

- [ ] **Step 1.4: Configure**

Run: `cmake -B build`
Expected: first run downloads JUCE + clap-juce-extensions (minutes), ends with `-- Generating done` / `-- Build files have been written to: .../build`. Failure mode "could not find any instance of Visual Studio" → Task 0 incomplete.

- [ ] **Step 1.5: Build all formats**

Run: `cmake --build build --config Release --parallel`
Expected: succeeds; artifacts exist:
- `build\VocalSibilance_artefacts\Release\VST3\Vocal Sibilance.vst3`
- `build\VocalSibilance_artefacts\Release\CLAP\Vocal Sibilance.clap`
- `build\VocalSibilance_artefacts\Release\Standalone\Vocal Sibilance.exe`

- [ ] **Step 1.6: Smoke-run the Standalone**

Run: `& "build\VocalSibilance_artefacts\Release\Standalone\Vocal Sibilance.exe"`
Expected: a window opens with generic JUCE audio settings; no crash. Close it.

- [ ] **Step 1.7: Commit**

```powershell
git add CMakeLists.txt src
git commit -m "feat: JUCE 8 scaffold - passthrough plugin builds VST3/CLAP/Standalone"
```

---

### Task 2: Test infrastructure (Catch2 + ctest)

**Files:**
- Modify: `CMakeLists.txt` (append at end)
- Create: `tests/SanityTests.cpp`

- [ ] **Step 2.1: Append test section to `CMakeLists.txt`**

```cmake
# ---------------- tests ----------------
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(Catch2)

enable_testing()

juce_add_console_app(dsp_tests PRODUCT_NAME "dsp_tests")

target_sources(dsp_tests PRIVATE
    tests/SanityTests.cpp)

target_include_directories(dsp_tests PRIVATE src)

target_compile_definitions(dsp_tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

target_link_libraries(dsp_tests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_dsp
    juce::juce_recommended_config_flags)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(dsp_tests)
```

- [ ] **Step 2.2: Write `tests/SanityTests.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>
#include <juce_dsp/juce_dsp.h>

TEST_CASE ("test infrastructure compiles and links JUCE dsp", "[sanity]")
{
    juce::SmoothedValue<float> sv;
    sv.setCurrentAndTargetValue (1.0f);
    REQUIRE (sv.getNextValue() == 1.0f);
}
```

- [ ] **Step 2.3: Build and run tests**

Run: `cmake -B build` then `cmake --build build --config Release --target dsp_tests --parallel` then `ctest --test-dir build -C Release --output-on-failure`
Expected: `100% tests passed, 0 tests failed out of 1`

- [ ] **Step 2.4: Commit**

```powershell
git add CMakeLists.txt tests
git commit -m "test: add Catch2 + ctest infrastructure"
```

---

### Task 3: CrossoverBands (LR4 band isolation)

**Files:**
- Create: `src/dsp/CrossoverBands.h`, `src/dsp/CrossoverBands.cpp`
- Create: `tests/CrossoverBandsTests.cpp`
- Modify: `CMakeLists.txt` (two `target_sources` blocks)

- [ ] **Step 3.1: Write the failing test `tests/CrossoverBandsTests.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include "dsp/CrossoverBands.h"

namespace
{
constexpr double pi = 3.141592653589793;

float steadyStateRmsRatio (float freqHz, double sr = 48000.0)
{
    vs::CrossoverBands xo;
    xo.prepare (sr, 512, 1);
    xo.setCrossoverFrequencies (4000.0f, 11000.0f);

    juce::AudioBuffer<float> in (1, 512), out (1, 512);
    double phase = 0.0;
    const double inc = 2.0 * pi * freqHz / sr;

    double sumIn = 0.0, sumOut = 0.0;
    const int warmupBlocks = 8, measureBlocks = 16;

    for (int b = 0; b < warmupBlocks + measureBlocks; ++b)
    {
        for (int i = 0; i < 512; ++i)
        {
            in.setSample (0, i, (float) std::sin (phase));
            phase += inc;
        }
        xo.split (in);
        out.clear();
        xo.recombine (out, 512);

        if (b >= warmupBlocks)
            for (int i = 0; i < 512; ++i)
            {
                sumIn  += (double) in.getSample (0, i) * in.getSample (0, i);
                sumOut += (double) out.getSample (0, i) * out.getSample (0, i);
            }
    }
    return (float) std::sqrt (sumOut / sumIn);
}
} // namespace

TEST_CASE ("LR4 split+recombine is magnitude-flat", "[crossover]")
{
    for (float f : { 100.0f, 1000.0f, 4000.0f, 8000.0f, 11000.0f, 16000.0f })
    {
        const float ratioDb = juce::Decibels::gainToDecibels (steadyStateRmsRatio (f));
        INFO ("freq " << f << " Hz, deviation " << ratioDb << " dB");
        CHECK (std::abs (ratioDb) < 0.15f);
    }
}

TEST_CASE ("crossover enforces a minimum half-octave gap", "[crossover]")
{
    vs::CrossoverBands xo;
    xo.prepare (48000.0, 64, 1);
    xo.setCrossoverFrequencies (8000.0f, 8000.0f);          // illegal: zero gap
    CHECK (xo.getHiHz() >= xo.getLoHz() * 1.41f);

    xo.setCrossoverFrequencies (16000.0f, 16000.0f);        // hi pinned at ceiling
    CHECK (xo.getHiHz() >= xo.getLoHz() * 1.41f);
}
```

- [ ] **Step 3.2: Add sources to `CMakeLists.txt`, build, verify test FAILS to compile**

In the `target_sources(VocalSibilance ...)` block add:

```cmake
    src/dsp/CrossoverBands.cpp
```

In the `target_sources(dsp_tests ...)` block add:

```cmake
    tests/CrossoverBandsTests.cpp
    src/dsp/CrossoverBands.cpp
```

Run: `cmake --build build --config Release --target dsp_tests --parallel`
Expected: FAIL — `Cannot open include file: 'dsp/CrossoverBands.h'`

- [ ] **Step 3.3: Write `src/dsp/CrossoverBands.h`**

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>

namespace vs
{
// Splits audio into low / sibilant band / high with 4th-order Linkwitz-Riley
// crossovers. recombine() of untouched bands is magnitude-flat (allpass phase).
class CrossoverBands
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void setCrossoverFrequencies (float loHz, float hiHz);

    void split (const juce::AudioBuffer<float>& input);
    void recombine (juce::AudioBuffer<float>& dest, int numSamples) const;

    juce::AudioBuffer<float>& getBand()             { return band; }
    const juce::AudioBuffer<float>& getLow()  const { return low; }
    const juce::AudioBuffer<float>& getHigh() const { return high; }

    float getLoHz() const { return loHz; }
    float getHiHz() const { return hiHz; }

    static constexpr float minHz = 2000.0f, maxHz = 16000.0f, minGapRatio = 1.4142f;

private:
    juce::dsp::LinkwitzRileyFilter<float> lowSplit, highSplit, lowAllpass;
    juce::AudioBuffer<float> low, band, high;
    float loHz = 4000.0f, hiHz = 11000.0f;
    int channels = 0;
};
} // namespace vs
```

- [ ] **Step 3.4: Write `src/dsp/CrossoverBands.cpp`**

```cpp
#include "CrossoverBands.h"

namespace vs
{
void CrossoverBands::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    channels = numChannels;
    const juce::dsp::ProcessSpec spec { sampleRate,
                                        (juce::uint32) maxBlockSize,
                                        (juce::uint32) numChannels };
    lowSplit.prepare (spec);
    highSplit.prepare (spec);
    lowAllpass.prepare (spec);
    lowAllpass.setType (juce::dsp::LinkwitzRileyFilterType::allpass);

    low.setSize (numChannels, maxBlockSize);
    band.setSize (numChannels, maxBlockSize);
    high.setSize (numChannels, maxBlockSize);

    setCrossoverFrequencies (loHz, hiHz);
    reset();
}

void CrossoverBands::reset()
{
    lowSplit.reset();
    highSplit.reset();
    lowAllpass.reset();
}

void CrossoverBands::setCrossoverFrequencies (float newLo, float newHi)
{
    loHz = juce::jlimit (minHz, maxHz, newLo);
    hiHz = juce::jlimit (minHz, maxHz, newHi);
    if (hiHz < loHz * minGapRatio)
        hiHz = juce::jmin (maxHz, loHz * minGapRatio);
    if (hiHz < loHz * minGapRatio)              // hi hit the ceiling: pull lo down
        loHz = hiHz / minGapRatio;

    lowSplit.setCutoffFrequency (loHz);
    highSplit.setCutoffFrequency (hiHz);
    lowAllpass.setCutoffFrequency (hiHz);
}

void CrossoverBands::split (const juce::AudioBuffer<float>& input)
{
    const int n = input.getNumSamples();
    for (int ch = 0; ch < channels; ++ch)
    {
        const float* src = input.getReadPointer (ch);
        float* lo = low.getWritePointer (ch);
        float* bd = band.getWritePointer (ch);
        float* hi = high.getWritePointer (ch);

        for (int i = 0; i < n; ++i)
        {
            float l = 0.0f, rest = 0.0f;
            lowSplit.processSample (ch, src[i], l, rest);
            l = lowAllpass.processSample (ch, l);   // phase-align with the hi split
            float b = 0.0f, h = 0.0f;
            highSplit.processSample (ch, rest, b, h);
            lo[i] = l; bd[i] = b; hi[i] = h;
        }
    }
}

void CrossoverBands::recombine (juce::AudioBuffer<float>& dest, int numSamples) const
{
    for (int ch = 0; ch < dest.getNumChannels(); ++ch)
    {
        dest.copyFrom (ch, 0, low,  ch, 0, numSamples);
        dest.addFrom  (ch, 0, band, ch, 0, numSamples);
        dest.addFrom  (ch, 0, high, ch, 0, numSamples);
    }
}
} // namespace vs
```

- [ ] **Step 3.5: Build, run tests, verify PASS**

Run: `cmake --build build --config Release --target dsp_tests --parallel` then `ctest --test-dir build -C Release --output-on-failure`
Expected: all tests pass (sanity + 2 crossover cases).

- [ ] **Step 3.6: Commit**

```powershell
git add CMakeLists.txt src/dsp tests
git commit -m "feat: LR4 CrossoverBands with flat recombine and range clamping"
```

---

### Task 4: SibilanceDetector

**Files:**
- Create: `src/dsp/SibilanceDetector.h`, `src/dsp/SibilanceDetector.cpp`
- Create: `tests/SibilanceDetectorTests.cpp`
- Modify: `CMakeLists.txt` (same two-block pattern as Task 3)

- [ ] **Step 4.1: Write the failing test `tests/SibilanceDetectorTests.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include "dsp/SibilanceDetector.h"

namespace
{
constexpr double pi = 3.141592653589793;
}

TEST_CASE ("detector opens on sibilant-dominated signal", "[detector]")
{
    vs::SibilanceDetector det;
    det.prepare (48000.0);

    float open = 0.0f;
    for (int i = 0; i < 4800; ++i)   // 100 ms of "ess": band energy == full energy
    {
        const float a = 0.3f * std::abs ((float) std::sin (2.0 * pi * 7000.0 * i / 48000.0));
        open = det.processSample (a, a);
    }
    CHECK (open > 0.7f);
}

TEST_CASE ("detector stays closed on vowel-dominated signal", "[detector]")
{
    vs::SibilanceDetector det;
    det.prepare (48000.0);

    float open = 0.0f;
    for (int i = 0; i < 9600; ++i)   // 200 ms: band energy ~ zero, full energy high
    {
        const float a = std::abs ((float) std::sin (2.0 * pi * 300.0 * i / 48000.0));
        open = det.processSample (0.001f * a, 0.3f * a);
    }
    CHECK (open < 0.1f);
}

TEST_CASE ("detector closes again after sibilance ends", "[detector]")
{
    vs::SibilanceDetector det;
    det.prepare (48000.0);

    float open = 0.0f;
    for (int i = 0; i < 4800; ++i)            // 100 ms ess
    {
        const float a = 0.3f * std::abs ((float) std::sin (2.0 * pi * 7000.0 * i / 48000.0));
        open = det.processSample (a, a);
    }
    REQUIRE (open > 0.7f);
    for (int i = 0; i < 9600; ++i)            // then 200 ms vowel
    {
        const float a = std::abs ((float) std::sin (2.0 * pi * 300.0 * i / 48000.0));
        open = det.processSample (0.001f * a, 0.3f * a);
    }
    CHECK (open < 0.15f);
}

TEST_CASE ("silence produces zero openness", "[detector]")
{
    vs::SibilanceDetector det;
    det.prepare (48000.0);
    float open = 1.0f;
    for (int i = 0; i < 1000; ++i)
        open = det.processSample (0.0f, 0.0f);
    CHECK (open == 0.0f);
}
```

- [ ] **Step 4.2: Add `tests/SibilanceDetectorTests.cpp` + `src/dsp/SibilanceDetector.cpp` to `dsp_tests` sources and `src/dsp/SibilanceDetector.cpp` to `VocalSibilance` sources; build; verify FAIL (missing header)**

- [ ] **Step 4.3: Write `src/dsp/SibilanceDetector.h`**

```cpp
#pragma once

namespace vs
{
// Converts per-sample band/full magnitudes into 0..1 "sibilance openness".
// Threshold-free: a fast band envelope is compared against a slower
// full-signal envelope, so behaviour tracks the singer's level automatically.
class SibilanceDetector
{
public:
    void prepare (double sampleRate);
    void reset();
    float processSample (float bandAbs, float fullAbs);   // returns openness 0..1

private:
    static float coeffForMs (float ms, double sampleRate);
    static void follow (float& env, float target, float attack, float release);

    float bandEnv = 0, fullEnv = 0, openness = 0;
    float bandAtt = 0, bandRel = 0, fullAtt = 0, fullRel = 0, openAtt = 0, openRel = 0;

    static constexpr float ratioLow   = 0.30f;   // openness 0 at/below this band/full ratio
    static constexpr float ratioHigh  = 0.85f;   // openness 1 at/above
    static constexpr float silenceEnv = 1.0e-3f; // ~ -60 dBFS: below this, stay closed
};
} // namespace vs
```

- [ ] **Step 4.4: Write `src/dsp/SibilanceDetector.cpp`**

```cpp
#include "SibilanceDetector.h"
#include <algorithm>
#include <cmath>

namespace vs
{
float SibilanceDetector::coeffForMs (float ms, double sampleRate)
{
    return 1.0f - std::exp (-1.0f / (0.001f * ms * (float) sampleRate));
}

void SibilanceDetector::prepare (double sr)
{
    bandAtt = coeffForMs (1.0f,   sr);
    bandRel = coeffForMs (60.0f,  sr);
    fullAtt = coeffForMs (15.0f,  sr);
    fullRel = coeffForMs (250.0f, sr);
    openAtt = coeffForMs (0.5f,   sr);
    openRel = coeffForMs (50.0f,  sr);
    reset();
}

void SibilanceDetector::reset()
{
    bandEnv = fullEnv = openness = 0.0f;
}

void SibilanceDetector::follow (float& env, float target, float attack, float release)
{
    env += (target > env ? attack : release) * (target - env);
}

float SibilanceDetector::processSample (float bandAbs, float fullAbs)
{
    follow (bandEnv, bandAbs, bandAtt, bandRel);
    follow (fullEnv, fullAbs, fullAtt, fullRel);

    float raw = 0.0f;
    if (fullEnv > silenceEnv)
    {
        const float ratio = bandEnv / fullEnv;
        raw = std::clamp ((ratio - ratioLow) / (ratioHigh - ratioLow), 0.0f, 1.0f);
        raw = raw * raw * (3.0f - 2.0f * raw);   // smoothstep knee
    }
    follow (openness, raw, openAtt, openRel);
    return openness;
}
} // namespace vs
```

- [ ] **Step 4.5: Build + ctest → all pass. Commit:**

```powershell
git add CMakeLists.txt src/dsp tests
git commit -m "feat: threshold-free SibilanceDetector (relative band/full envelopes)"
```

---

### Task 5: GainComputer

**Files:**
- Create: `src/dsp/GainComputer.h`, `src/dsp/GainComputer.cpp`
- Create: `tests/GainComputerTests.cpp`
- Modify: `CMakeLists.txt` (same two-block pattern)

- [ ] **Step 5.1: Write the failing test `tests/GainComputerTests.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "dsp/GainComputer.h"

TEST_CASE ("full openness at full smooth approaches max depth (-15 dB)", "[gain]")
{
    vs::GainComputer gc;
    gc.prepare (48000.0);
    gc.setSmoothAmount (1.0f);

    float g = 1.0f;
    for (int i = 0; i < 4800; ++i)      // 100 ms, plenty for the 1 ms attack
        g = gc.processSample (1.0f);

    CHECK (g == Catch::Approx (0.17783f).margin (0.005f));   // 10^(-15/20)
}

TEST_CASE ("zero smooth leaves gain at exactly unity", "[gain]")
{
    vs::GainComputer gc;
    gc.prepare (48000.0);
    gc.setSmoothAmount (0.0f);

    float g = 0.0f;
    for (int i = 0; i < 100; ++i)
        g = gc.processSample (1.0f);

    CHECK (g == 1.0f);
}

TEST_CASE ("gain recovers when openness closes", "[gain]")
{
    vs::GainComputer gc;
    gc.prepare (48000.0);
    gc.setSmoothAmount (1.0f);

    for (int i = 0; i < 4800; ++i) gc.processSample (1.0f);
    float g = 0.0f;
    for (int i = 0; i < 9600; ++i) g = gc.processSample (0.0f);   // 200 ms release
    CHECK (g > 0.95f);
}
```

- [ ] **Step 5.2: Add sources to CMake (both targets), build, verify FAIL**

- [ ] **Step 5.3: Write `src/dsp/GainComputer.h`**

```cpp
#pragma once

namespace vs
{
// Maps detector openness + the user's Smooth amount onto a smoothed linear
// gain (1.0 down to ~0.178 = -15 dB) for the sibilant band.
class GainComputer
{
public:
    static constexpr float maxDepthDb = 15.0f;

    void prepare (double sampleRate);
    void reset();
    void setSmoothAmount (float amount01);
    float processSample (float openness);   // returns linear band gain

private:
    float smoothAmount = 0.35f;
    float gain = 1.0f;
    float attCoeff = 0.0f, relCoeff = 0.0f;
};
} // namespace vs
```

- [ ] **Step 5.4: Write `src/dsp/GainComputer.cpp`**

```cpp
#include "GainComputer.h"
#include <algorithm>
#include <cmath>

namespace vs
{
namespace
{
    float coeffForMs (float ms, double sampleRate)
    {
        return 1.0f - std::exp (-1.0f / (0.001f * ms * (float) sampleRate));
    }
} // namespace

void GainComputer::prepare (double sr)
{
    attCoeff = coeffForMs (1.0f,  sr);   // clamp down fast
    relCoeff = coeffForMs (30.0f, sr);   // recover gently
    reset();
}

void GainComputer::reset()
{
    gain = 1.0f;
}

void GainComputer::setSmoothAmount (float a)
{
    smoothAmount = std::clamp (a, 0.0f, 1.0f);
}

float GainComputer::processSample (float openness)
{
    const float knee = openness * openness * (3.0f - 2.0f * openness);  // soft knee
    const float targetDb = -maxDepthDb * smoothAmount * knee;
    const float target = std::pow (10.0f, targetDb * (1.0f / 20.0f));
    gain += (target < gain ? attCoeff : relCoeff) * (target - gain);
    return gain;
}
} // namespace vs
```

- [ ] **Step 5.5: Build + ctest → all pass. Commit:**

```powershell
git add CMakeLists.txt src/dsp tests
git commit -m "feat: soft-knee GainComputer with fast-attack/gentle-release smoothing"
```

---

### Task 6: GritShaper (morphing waveshaper, 8× oversampled)

**Files:**
- Create: `src/dsp/GritShaper.h`, `src/dsp/GritShaper.cpp`
- Create: `tests/GritShaperTests.cpp`
- Modify: `CMakeLists.txt` (same two-block pattern)

- [ ] **Step 6.1: Write the failing test `tests/GritShaperTests.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>
#include <vector>
#include "dsp/GritShaper.h"

namespace
{
constexpr double pi = 3.141592653589793;
}

TEST_CASE ("shaper has unity small-signal gain at character 0", "[grit]")
{
    vs::GritShaper gs;
    gs.prepare (48000.0, 512, 1);
    gs.setCharacter (0.0f);
    CHECK (gs.shapeForTest (0.001f, 1.0f) == Catch::Approx (0.001f).epsilon (0.01));
    CHECK (gs.shapeForTest (0.001f, 10.0f) == Catch::Approx (0.001f).epsilon (0.05));
}

TEST_CASE ("grit at zero is a bit-exact passthrough", "[grit]")
{
    vs::GritShaper gs;
    gs.prepare (48000.0, 512, 1);
    gs.setGrit (0.0f);

    juce::AudioBuffer<float> buf (1, 512);
    std::vector<float> env (512, 1.0f);
    for (int i = 0; i < 512; ++i)
        buf.setSample (0, i, (float) std::sin (2.0 * pi * 5000.0 * i / 48000.0));
    juce::AudioBuffer<float> ref;
    ref.makeCopyOf (buf);

    gs.process (buf, env.data(), 512);

    for (int i = 0; i < 512; ++i)
        REQUIRE (buf.getSample (0, i) == ref.getSample (0, i));
}

TEST_CASE ("reported latency is near zero (minimum-phase IIR)", "[grit]")
{
    vs::GritShaper gs;
    gs.prepare (48000.0, 512, 1);
    CHECK (gs.getLatencySamples() < 4.0f);
}

TEST_CASE ("aliasing sits >= 40 dB below the loudest harmonic", "[grit]")
{
    constexpr double sr = 48000.0;
    constexpr int N = 1 << 14;

    vs::GritShaper gs;
    gs.prepare (sr, 512, 1);
    gs.setGrit (1.0f);
    gs.setCharacter (1.0f);

    std::vector<float> env (512, 1.0f);
    juce::AudioBuffer<float> buf (1, 512);
    std::vector<float> captured;
    double phase = 0.0;
    const double inc = 2.0 * pi * 9000.0 / sr;     // 9 kHz -> bin-exact at N=16384

    while ((int) captured.size() < N + 4096)        // 4096 warmup + N analysed
    {
        for (int i = 0; i < 512; ++i)
        {
            buf.setSample (0, i, 0.5f * (float) std::sin (phase));
            phase += inc;
        }
        gs.process (buf, env.data(), 512);
        for (int i = 0; i < 512; ++i)
            captured.push_back (buf.getSample (0, i));
    }

    std::vector<float> fft (2 * N, 0.0f);
    std::copy (captured.end() - N, captured.end(), fft.begin());
    juce::dsp::WindowingFunction<float> win ((size_t) N,
        juce::dsp::WindowingFunction<float>::hann);
    win.multiplyWithWindowingTable (fft.data(), (size_t) N);
    juce::dsp::FFT engine (14);
    engine.performFrequencyOnlyForwardTransform (fft.data());

    auto binOf = [&] (double f) { return (int) std::round (f * N / sr); };
    auto peakAround = [&] (int bin)
    {
        float m = 0.0f;
        for (int k = bin - 4; k <= bin + 4; ++k)
            m = std::max (m, fft[(size_t) k]);
        return m;
    };

    const float maxHarm = std::max (peakAround (binOf (9000.0)),
                                    peakAround (binOf (18000.0)));

    float maxSpur = 0.0f;
    for (int k = binOf (100.0); k < binOf (23000.0); ++k)
    {
        if (std::abs (k - binOf (9000.0)) <= 6 || std::abs (k - binOf (18000.0)) <= 6)
            continue;
        maxSpur = std::max (maxSpur, fft[(size_t) k]);
    }

    const float dbDown = juce::Decibels::gainToDecibels (maxSpur / maxHarm);
    INFO ("worst spur is " << dbDown << " dB below loudest harmonic");
    CHECK (dbDown < -40.0f);
}
```

- [ ] **Step 6.2: Add sources to CMake (both targets), build, verify FAIL**

- [ ] **Step 6.3: Write `src/dsp/GritShaper.h`**

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>

namespace vs
{
// Detector-gated morphing waveshaper for the sibilant band, 8x oversampled
// with minimum-phase polyphase IIR halfbands (no integer latency).
class GritShaper
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void setGrit (float amount01);
    void setCharacter (float amount01);
    float getLatencySamples() const;

    // env: one openness value per *base-rate* sample, shared by all channels
    void process (juce::AudioBuffer<float>& band, const float* env, int numSamples);

    float shapeForTest (float x, float drive) const { return shape (x, drive); }

private:
    float shape (float x, float drive) const;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    std::vector<juce::dsp::IIR::Filter<float>> dcBlockers;
    float grit = 0.0f, character = 0.3f;
    bool wasActive = false;
};
} // namespace vs
```

- [ ] **Step 6.4: Write `src/dsp/GritShaper.cpp`**

```cpp
#include "GritShaper.h"
#include <cmath>

namespace vs
{
void GritShaper::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numChannels, 3,                       // 3 stages -> 8x
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        false);                                        // not max quality: min phase
    oversampling->initProcessing ((size_t) maxBlockSize);

    dcBlockers.clear();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        juce::dsp::IIR::Filter<float> f;
        f.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 20.0f);
        dcBlockers.push_back (f);
    }
    reset();
}

void GritShaper::reset()
{
    if (oversampling != nullptr)
        oversampling->reset();
    for (auto& f : dcBlockers)
        f.reset();
    wasActive = false;
}

void GritShaper::setGrit (float a)      { grit = juce::jlimit (0.0f, 1.0f, a); }
void GritShaper::setCharacter (float a) { character = juce::jlimit (0.0f, 1.0f, a); }

float GritShaper::getLatencySamples() const
{
    return oversampling != nullptr ? oversampling->getLatencyInSamples() : 0.0f;
}

float GritShaper::shape (float x, float drive) const
{
    const float sym  = std::tanh (drive * x);              // tape-ish rounding
    const float bent = x + 0.35f * x * std::abs (x);       // even-order content
    const float asym = std::tanh (drive * 1.3f * bent);    // edgier lobe
    return (sym + character * (asym - sym)) / drive;       // unity small-signal gain
}

void GritShaper::process (juce::AudioBuffer<float>& band, const float* env, int numSamples)
{
    if (grit < 1.0e-4f)
    {
        if (wasActive)        // leaving the active state: clear filter history so
            reset();          // the next engage starts clean (no stale ringing)
        return;
    }
    wasActive = true;

    juce::dsp::AudioBlock<float> block (band.getArrayOfWritePointers(),
                                        (size_t) band.getNumChannels(),
                                        (size_t) numSamples);
    auto osBlock = oversampling->processSamplesUp (block);

    for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
    {
        float* d = osBlock.getChannelPointer (ch);
        for (size_t i = 0; i < osBlock.getNumSamples(); ++i)
        {
            const float wet = grit * env[i >> 3];          // 8x: base-rate index i/8
            if (wet <= 0.0f)
                continue;
            const float drive = 1.0f + 9.0f * wet;
            d[i] += wet * (shape (d[i], drive) - d[i]);
        }
    }

    oversampling->processSamplesDown (block);

    for (int ch = 0; ch < band.getNumChannels(); ++ch)     // kill DC from the asym curve
    {
        float* p = band.getWritePointer (ch);
        auto& f = dcBlockers[(size_t) ch];
        for (int i = 0; i < numSamples; ++i)
            p[i] = f.processSample (p[i]);
    }
}
} // namespace vs
```

- [ ] **Step 6.5: Build + ctest → all pass (the aliasing case is the slow one, ~seconds). Commit:**

```powershell
git add CMakeLists.txt src/dsp tests
git commit -m "feat: 4x oversampled morphing GritShaper with DC blocking and aliasing test"
```

---

### Task 7: SibilanceEngine (wires the chain)

**Files:**
- Create: `src/dsp/SibilanceEngine.h`, `src/dsp/SibilanceEngine.cpp`
- Create: `tests/SibilanceEngineTests.cpp`
- Modify: `CMakeLists.txt` (same two-block pattern)

Key invariant implemented here: when Smooth and Grit are both 0 (or bypass is on), a crossfade (`wetFade`) lands on the **untouched dry copy**, making the output bit-exact — `out = dry + w*(proc-dry)` with `w == 0`.

- [ ] **Step 7.1: Write the failing test `tests/SibilanceEngineTests.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>
#include "dsp/SibilanceEngine.h"

namespace
{
constexpr double pi = 3.141592653589793;

vs::EngineParams neutralParams()
{
    vs::EngineParams p;
    p.smooth = 0.0f; p.grit = 0.0f; p.character = 0.3f;
    p.mix = 1.0f; p.outputDb = 0.0f;
    p.rangeLoHz = 4000.0f; p.rangeHiHz = 11000.0f;
    p.listen = false; p.bypassed = false;
    return p;
}

void fillSine (juce::AudioBuffer<float>& b, double freq, double sr, double& phase, float amp)
{
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const float v = amp * (float) std::sin (phase);
        phase += 2.0 * pi * freq / sr;
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            b.setSample (ch, i, v);
    }
}

double rmsOf (const std::vector<float>& v, size_t from, size_t to)
{
    double s = 0.0;
    for (size_t i = from; i < to; ++i)
        s += (double) v[i] * v[i];
    return std::sqrt (s / (double) (to - from));
}
} // namespace

TEST_CASE ("neutral settings are bit-transparent", "[engine]")
{
    vs::SibilanceEngine eng;
    eng.prepare (48000.0, 512, 2);
    eng.setParams (neutralParams());

    juce::Random rng (42);
    for (int block = 0; block < 8; ++block)
    {
        juce::AudioBuffer<float> buf (2, 512), ref (2, 512);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                buf.setSample (ch, i, rng.nextFloat() * 1.6f - 0.8f);
        ref.makeCopyOf (buf);

        eng.process (buf);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                REQUIRE (buf.getSample (ch, i) == ref.getSample (ch, i));
    }
}

TEST_CASE ("engine reduces a sibilant burst but not the vowel", "[engine]")
{
    constexpr double sr = 48000.0;
    vs::SibilanceEngine eng;
    eng.prepare (sr, 512, 1);
    auto p = neutralParams();
    p.smooth = 1.0f;
    eng.setParams (p);

    const int vowelN = 14400, essN = 7200;   // 300 ms @300 Hz, then 150 ms @8 kHz
    std::vector<float> in, out;
    double phase = 0.0;
    juce::AudioBuffer<float> buf (1, 512);

    auto run = [&] (double freq, float amp, int total)
    {
        int done = 0;
        while (done < total)
        {
            const int n = std::min (512, total - done);
            buf.setSize (1, n, false, false, true);
            fillSine (buf, freq, sr, phase, amp);
            for (int i = 0; i < n; ++i) in.push_back (buf.getSample (0, i));
            eng.process (buf);
            for (int i = 0; i < n; ++i) out.push_back (buf.getSample (0, i));
            done += n;
        }
    };
    run (300.0, 0.5f, vowelN);
    run (8000.0, 0.4f, essN);

    // vowel: steady tail (skip first 100 ms = crossfade + filter settle)
    const double vowelInDb  = 20.0 * std::log10 (rmsOf (in,  4800, (size_t) vowelN));
    const double vowelOutDb = 20.0 * std::log10 (rmsOf (out, 4800, (size_t) vowelN));
    CHECK (std::abs (vowelOutDb - vowelInDb) < 0.25);

    // ess: skip 20 ms attack, then expect >= 6 dB average reduction
    const size_t essFrom = (size_t) vowelN + 960, essTo = (size_t) (vowelN + essN);
    const double essInDb  = 20.0 * std::log10 (rmsOf (in,  essFrom, essTo));
    const double essOutDb = 20.0 * std::log10 (rmsOf (out, essFrom, essTo));
    CHECK (essInDb - essOutDb > 6.0);

    CHECK (eng.getMetrics().gainReductionDb > 6.0f);
}

TEST_CASE ("listen solos the band", "[engine]")
{
    // a 300 Hz tone is far below the band: listen output must be near-silent
    constexpr double sr = 48000.0;
    vs::SibilanceEngine eng;
    eng.prepare (sr, 512, 1);
    auto p = neutralParams();
    p.listen = true;
    eng.setParams (p);

    juce::AudioBuffer<float> buf (1, 512);
    double phase = 0.0, sumIn = 0.0, sumOut = 0.0;
    for (int b = 0; b < 16; ++b)
    {
        fillSine (buf, 300.0, sr, phase, 0.5f);
        if (b >= 8)
            for (int i = 0; i < 512; ++i)
                sumIn += (double) buf.getSample (0, i) * buf.getSample (0, i);
        eng.process (buf);
        if (b >= 8)
            for (int i = 0; i < 512; ++i)
                sumOut += (double) buf.getSample (0, i) * buf.getSample (0, i);
    }
    CHECK (std::sqrt (sumOut) < std::sqrt (sumIn) * 0.1);   // > 20 dB down
}

TEST_CASE ("bypass is bit-transparent even with smooth up", "[engine]")
{
    vs::SibilanceEngine eng;
    eng.prepare (48000.0, 512, 2);
    auto p = neutralParams();
    p.smooth = 1.0f;
    p.bypassed = true;
    eng.setParams (p);

    juce::Random rng (7);
    juce::AudioBuffer<float> buf (2, 512), ref (2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buf.setSample (ch, i, rng.nextFloat() - 0.5f);
    ref.makeCopyOf (buf);

    eng.process (buf);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            REQUIRE (buf.getSample (ch, i) == ref.getSample (ch, i));
}
```

- [ ] **Step 7.2: Add sources to CMake (both targets), build, verify FAIL**

- [ ] **Step 7.3: Write `src/dsp/SibilanceEngine.h`**

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include "CrossoverBands.h"
#include "SibilanceDetector.h"
#include "GainComputer.h"
#include "GritShaper.h"

namespace vs
{
struct EngineParams
{
    float smooth = 0.35f;        // 0..1
    float grit = 0.0f;           // 0..1
    float character = 0.3f;      // 0..1
    float mix = 1.0f;            // 0..1
    float outputDb = 0.0f;       // -12..+12
    float rangeLoHz = 4000.0f;
    float rangeHiHz = 11000.0f;
    bool listen = false;
    bool bypassed = false;
};

struct EngineMetrics
{
    float openness = 0.0f;          // block-average detector openness 0..1
    float gainReductionDb = 0.0f;   // peak reduction this block (positive dB)
};

class SibilanceEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void setParams (const EngineParams& p);   // once per block, from cached atomics
    void process (juce::AudioBuffer<float>& buffer);

    EngineMetrics getMetrics() const { return metrics; }
    int getLatencySamples() const    { return 0; }

private:
    CrossoverBands crossover;
    SibilanceDetector detector;
    GainComputer gainComputer;
    GritShaper gritShaper;

    juce::AudioBuffer<float> dryBuffer;
    std::vector<float> envBuffer;

    juce::SmoothedValue<float> wetFade;       // 0 -> bit-exact dry (neutral/bypass)
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> outputGain;

    EngineParams params;
    EngineMetrics metrics;
};
} // namespace vs
```

- [ ] **Step 7.4: Write `src/dsp/SibilanceEngine.cpp`**

```cpp
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

    reset();
}

void SibilanceEngine::reset()
{
    crossover.reset();
    detector.reset();
    gainComputer.reset();
    gritShaper.reset();
}

void SibilanceEngine::setParams (const EngineParams& p)
{
    params = p;
    gainComputer.setSmoothAmount (p.smooth);
    gritShaper.setGrit (p.grit);
    gritShaper.setCharacter (p.character);
    crossover.setCrossoverFrequencies (p.rangeLoHz, p.rangeHiHz);
}

void SibilanceEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels();
    const int n = buffer.getNumSamples();

    for (int ch = 0; ch < numCh; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, n);

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
```

- [ ] **Step 7.5: Build + ctest → all pass. Commit:**

```powershell
git add CMakeLists.txt src/dsp tests
git commit -m "feat: SibilanceEngine - full chain with bit-exact neutral/bypass crossfade"
```

---

### Task 8: Real PluginProcessor (APVTS, state, FIFO, metrics)

Replaces the Task 1 stub entirely. The custom editor doesn't exist yet, so `createEditor()` is guarded by `VS_USE_GENERIC_EDITOR` (=1 for now on both targets; Task 13 removes it from the plugin target only — the test target keeps it forever so tests never link UI code).

**Files:**
- Create: `src/state/SpectrumFifo.h`
- Rewrite: `src/PluginProcessor.h`, `src/PluginProcessor.cpp`
- Create: `tests/ProcessorTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 8.1: Write the failing test `tests/ProcessorTests.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "PluginProcessor.h"

namespace
{
void setParam (VocalSibilanceProcessor& proc, const char* id, float plainValue)
{
    auto* p = proc.apvts.getParameter (id);
    REQUIRE (p != nullptr);
    p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}
} // namespace

TEST_CASE ("state save/restore round-trips parameter values", "[processor]")
{
    VocalSibilanceProcessor a;
    setParam (a, "smooth", 77.0f);
    setParam (a, "rangeLo", 3000.0f);
    setParam (a, "listen", 1.0f);

    juce::MemoryBlock blob;
    a.getStateInformation (blob);

    VocalSibilanceProcessor b;
    b.setStateInformation (blob.getData(), (int) blob.getSize());

    CHECK (b.apvts.getRawParameterValue ("smooth")->load() == Catch::Approx (77.0f).margin (0.01));
    CHECK (b.apvts.getRawParameterValue ("rangeLo")->load() == Catch::Approx (3000.0f).margin (1.0));
    CHECK (b.apvts.getRawParameterValue ("listen")->load() == 1.0f);
}

TEST_CASE ("corrupt state blobs are ignored without crashing", "[processor]")
{
    VocalSibilanceProcessor p;
    const char junk[] = "not a valid state";
    p.setStateInformation (junk, (int) sizeof (junk));
    CHECK (p.apvts.getRawParameterValue ("smooth")->load() == Catch::Approx (35.0f).margin (0.01));
}

TEST_CASE ("processBlock at neutral is bit-transparent", "[processor]")
{
    VocalSibilanceProcessor proc;
    setParam (proc, "smooth", 0.0f);
    setParam (proc, "grit", 0.0f);
    proc.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buf (2, 512), ref (2, 512);
    juce::Random rng (1234);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buf.setSample (ch, i, rng.nextFloat() - 0.5f);
    ref.makeCopyOf (buf);

    juce::MidiBuffer midi;
    proc.processBlock (buf, midi);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            REQUIRE (buf.getSample (ch, i) == ref.getSample (ch, i));
}

TEST_CASE ("mono and stereo layouts accepted, mismatch rejected", "[processor]")
{
    VocalSibilanceProcessor p;
    juce::AudioProcessor::BusesLayout mono, stereo, mismatch;
    mono.inputBuses.add (juce::AudioChannelSet::mono());
    mono.outputBuses.add (juce::AudioChannelSet::mono());
    stereo.inputBuses.add (juce::AudioChannelSet::stereo());
    stereo.outputBuses.add (juce::AudioChannelSet::stereo());
    mismatch.inputBuses.add (juce::AudioChannelSet::mono());
    mismatch.outputBuses.add (juce::AudioChannelSet::stereo());

    CHECK (p.setBusesLayout (mono));
    CHECK (p.setBusesLayout (stereo));
    CHECK (! p.setBusesLayout (mismatch));
}
```

- [ ] **Step 8.2: CMake — add test file and editor guard; verify FAIL**

Add to `target_sources(dsp_tests ...)`: `tests/ProcessorTests.cpp` and `src/PluginProcessor.cpp`.
Add to `target_link_libraries(dsp_tests ...)`: `juce::juce_audio_utils` (above `juce::juce_dsp`).
Add after the dsp_tests definitions block:

```cmake
target_compile_definitions(VocalSibilance PRIVATE VS_USE_GENERIC_EDITOR=1)  # removed in Task 13
target_compile_definitions(dsp_tests PRIVATE VS_USE_GENERIC_EDITOR=1)       # permanent
```

Build → FAIL (`apvts` doesn't exist yet on the stub).

- [ ] **Step 8.3: Write `src/state/SpectrumFifo.h`**

```cpp
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
```

- [ ] **Step 8.4: Rewrite `src/PluginProcessor.h`**

```cpp
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

    std::atomic<float>* pSmooth = nullptr;
    std::atomic<float>* pGrit = nullptr;
    std::atomic<float>* pCharacter = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pRangeLo = nullptr;
    std::atomic<float>* pRangeHi = nullptr;
    std::atomic<float>* pListen = nullptr;
    std::atomic<float>* pBypass = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalSibilanceProcessor)
};
```

- [ ] **Step 8.5: Rewrite `src/PluginProcessor.cpp`**

```cpp
#include "PluginProcessor.h"
#if ! VS_USE_GENERIC_EDITOR
 #include "ui/PluginEditor.h"
#endif

juce::AudioProcessorValueTreeState::ParameterLayout
VocalSibilanceProcessor::createParameterLayout()
{
    using FloatParam = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = [] (const char* id, const char* name, float defaultValue)
    {
        return std::make_unique<FloatParam> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), defaultValue,
            juce::AudioParameterFloatAttributes().withLabel ("%"));
    };

    layout.add (pct ("smooth",    "Smooth",    35.0f));
    layout.add (pct ("grit",      "Grit",       0.0f));
    layout.add (pct ("character", "Character", 30.0f));
    layout.add (pct ("mix",       "Mix",      100.0f));
    layout.add (std::make_unique<FloatParam> (
        juce::ParameterID { "output", 1 }, "Output",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    const juce::NormalisableRange<float> freqRange (2000.0f, 16000.0f, 1.0f, 0.5f);
    layout.add (std::make_unique<FloatParam> (
        juce::ParameterID { "rangeLo", 1 }, "Range Low", freqRange, 4000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    layout.add (std::make_unique<FloatParam> (
        juce::ParameterID { "rangeHi", 1 }, "Range High", freqRange, 11000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "listen", 1 }, "Listen", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return layout;
}

VocalSibilanceProcessor::VocalSibilanceProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    pSmooth    = apvts.getRawParameterValue ("smooth");
    pGrit      = apvts.getRawParameterValue ("grit");
    pCharacter = apvts.getRawParameterValue ("character");
    pMix       = apvts.getRawParameterValue ("mix");
    pOutput    = apvts.getRawParameterValue ("output");
    pRangeLo   = apvts.getRawParameterValue ("rangeLo");
    pRangeHi   = apvts.getRawParameterValue ("rangeHi");
    pListen    = apvts.getRawParameterValue ("listen");
    pBypass    = apvts.getRawParameterValue ("bypass");
}

bool VocalSibilanceProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out)
        return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void VocalSibilanceProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock,
                    juce::jmax (1, getTotalNumInputChannels()));
    setLatencySamples (engine.getLatencySamples());
}

void VocalSibilanceProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    spectrumFifo.push (buffer);          // pre-processing input for the display

    vs::EngineParams p;
    p.smooth     = pSmooth->load()    * 0.01f;
    p.grit       = pGrit->load()      * 0.01f;
    p.character  = pCharacter->load() * 0.01f;
    p.mix        = pMix->load()       * 0.01f;
    p.outputDb   = pOutput->load();
    p.rangeLoHz  = pRangeLo->load();
    p.rangeHiHz  = pRangeHi->load();
    p.listen     = pListen->load() > 0.5f;
    p.bypassed   = pBypass->load() > 0.5f;

    engine.setParams (p);
    engine.process (buffer);

    const auto m = engine.getMetrics();
    uiOpenness.store (m.openness);
    uiGainReduction.store (m.gainReductionDb);
}

juce::AudioProcessorEditor* VocalSibilanceProcessor::createEditor()
{
#if VS_USE_GENERIC_EDITOR
    return new juce::GenericAudioProcessorEditor (*this);
#else
    return new VocalSibilanceEditor (*this);
#endif
}

void VocalSibilanceProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("stateVersion", stateVersion, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalSibilanceProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    // unknown/corrupt blobs and missing fields: keep current/default values
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalSibilanceProcessor();
}
```

- [ ] **Step 8.6: Build + ctest → all pass (engine + processor + dsp suites). Commit:**

```powershell
git add CMakeLists.txt src tests
git commit -m "feat: real PluginProcessor - APVTS params, versioned state, spectrum FIFO, metrics"
```

- [ ] **Step 8.7: Functional milestone check (generic UI)**

Run: `cmake --build build --config Release --parallel` then launch the Standalone exe.
Expected: generic slider UI with all 9 parameters; speaking into the mic with Smooth up audibly tames "s" sounds. This is the first usable build.

---

### Task 9: Assets — Inter font, factory presets, BinaryData target

**Files:**
- Create: `assets/fonts/InterRegular.ttf`, `assets/fonts/InterSemiBold.ttf`, `assets/fonts/InterLicense-OFL.txt` (downloaded)
- Create: `presets/Gentle.xml`, `presets/Standard.xml`, `presets/BrightVocal.xml`, `presets/TameHarshMic.xml`, `presets/GrittyPop.xml`, `presets/MaximumCharacter.xml`
- Modify: `CMakeLists.txt`

- [ ] **Step 9.1: Download Inter (OFL-licensed) and extract the two static TTFs**

```powershell
New-Item -ItemType Directory -Force assets\fonts | Out-Null
curl.exe -L -o inter.zip https://github.com/rsms/inter/releases/download/v4.1/Inter-4.1.zip
Expand-Archive inter.zip -DestinationPath inter_tmp
Copy-Item "inter_tmp\extras\ttf\Inter-Regular.ttf"  assets\fonts\InterRegular.ttf
Copy-Item "inter_tmp\extras\ttf\Inter-SemiBold.ttf" assets\fonts\InterSemiBold.ttf
Copy-Item "inter_tmp\LICENSE.txt" assets\fonts\InterLicense-OFL.txt
Remove-Item -Recurse -Force inter_tmp, inter.zip
```

If the archive layout differs in a newer release, find the **static** (non-variable) `Inter-Regular.ttf` / `Inter-SemiBold.ttf` inside it and copy them to those exact destination names — the BinaryData symbol names below depend on them.

- [ ] **Step 9.2: Write the six factory preset XML files**

`presets/Gentle.xml`:

```xml
<Preset name="Gentle">
  <PARAM id="smooth" value="25"/>
  <PARAM id="grit" value="0"/>
  <PARAM id="character" value="30"/>
  <PARAM id="mix" value="100"/>
  <PARAM id="output" value="0"/>
  <PARAM id="rangeLo" value="4000"/>
  <PARAM id="rangeHi" value="10000"/>
</Preset>
```

`presets/Standard.xml`:

```xml
<Preset name="Standard">
  <PARAM id="smooth" value="40"/>
  <PARAM id="grit" value="10"/>
  <PARAM id="character" value="30"/>
  <PARAM id="mix" value="100"/>
  <PARAM id="output" value="0"/>
  <PARAM id="rangeLo" value="4000"/>
  <PARAM id="rangeHi" value="11000"/>
</Preset>
```

`presets/BrightVocal.xml`:

```xml
<Preset name="Bright Vocal">
  <PARAM id="smooth" value="35"/>
  <PARAM id="grit" value="5"/>
  <PARAM id="character" value="15"/>
  <PARAM id="mix" value="100"/>
  <PARAM id="output" value="0"/>
  <PARAM id="rangeLo" value="5000"/>
  <PARAM id="rangeHi" value="12000"/>
</Preset>
```

`presets/TameHarshMic.xml`:

```xml
<Preset name="Tame Harsh Mic">
  <PARAM id="smooth" value="60"/>
  <PARAM id="grit" value="0"/>
  <PARAM id="character" value="30"/>
  <PARAM id="mix" value="100"/>
  <PARAM id="output" value="0"/>
  <PARAM id="rangeLo" value="3500"/>
  <PARAM id="rangeHi" value="12000"/>
</Preset>
```

`presets/GrittyPop.xml`:

```xml
<Preset name="Gritty Pop">
  <PARAM id="smooth" value="45"/>
  <PARAM id="grit" value="35"/>
  <PARAM id="character" value="55"/>
  <PARAM id="mix" value="100"/>
  <PARAM id="output" value="0"/>
  <PARAM id="rangeLo" value="4000"/>
  <PARAM id="rangeHi" value="11000"/>
</Preset>
```

`presets/MaximumCharacter.xml`:

```xml
<Preset name="Maximum Character">
  <PARAM id="smooth" value="50"/>
  <PARAM id="grit" value="70"/>
  <PARAM id="character" value="85"/>
  <PARAM id="mix" value="90"/>
  <PARAM id="output" value="0"/>
  <PARAM id="rangeLo" value="3000"/>
  <PARAM id="rangeHi" value="13000"/>
</Preset>
```

- [ ] **Step 9.3: Add the BinaryData target to `CMakeLists.txt`** (insert after the `clap_juce_extensions_plugin(...)` call)

```cmake
juce_add_binary_data(VSAssets SOURCES
    assets/fonts/InterRegular.ttf
    assets/fonts/InterSemiBold.ttf
    presets/Gentle.xml
    presets/Standard.xml
    presets/BrightVocal.xml
    presets/TameHarshMic.xml
    presets/GrittyPop.xml
    presets/MaximumCharacter.xml)
```

And add `VSAssets` to the PRIVATE section of `target_link_libraries(VocalSibilance ...)`. Generated symbols: `BinaryData::InterRegular_ttf`, `BinaryData::Gentle_xml`, etc.

- [ ] **Step 9.4: Build (plugin target) → still succeeds. Commit:**

```powershell
git add CMakeLists.txt assets presets
git commit -m "feat: bundle Inter font (OFL) and six factory presets as BinaryData"
```

---

### Task 10: PorcelainLookAndFeel + PorcelainKnob

UI code is verified by building + the visual checks in Task 13. These sources go into the **plugin target only**, never `dsp_tests`.

**Files:**
- Create: `src/ui/PorcelainLookAndFeel.h`, `src/ui/PorcelainLookAndFeel.cpp`
- Create: `src/ui/PorcelainKnob.h`
- Modify: `CMakeLists.txt` (add `src/ui/PorcelainLookAndFeel.cpp` to `target_sources(VocalSibilance ...)`)

- [ ] **Step 10.1: Write `src/ui/PorcelainLookAndFeel.h`**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace vs
{
namespace porcelain
{
    const juce::Colour face    { 0xFFF7F6F3 };
    const juce::Colour inset   { 0xFFEFEDE8 };
    const juce::Colour line    { 0xFFDCD9D2 };
    const juce::Colour barIdle { 0xFFD5D2CA };
    const juce::Colour text    { 0xFF2A2A28 };
    const juce::Colour muted   { 0xFF6B6862 };
    const juce::Colour accent  { 0xFFE0552C };
}

class PorcelainLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PorcelainLookAndFeel();

    juce::Font labelFont (float height) const;
    juce::Font titleFont (float height) const;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

private:
    juce::Typeface::Ptr regular, semiBold;
};
} // namespace vs
```

- [ ] **Step 10.2: Write `src/ui/PorcelainLookAndFeel.cpp`**

```cpp
#include "PorcelainLookAndFeel.h"
#include "BinaryData.h"

namespace vs
{
PorcelainLookAndFeel::PorcelainLookAndFeel()
{
    regular  = juce::Typeface::createSystemTypefaceFor (BinaryData::InterRegular_ttf,
                                                        BinaryData::InterRegular_ttfSize);
    semiBold = juce::Typeface::createSystemTypefaceFor (BinaryData::InterSemiBold_ttf,
                                                        BinaryData::InterSemiBold_ttfSize);
    setDefaultSansSerifTypeface (regular);

    setColour (juce::ResizableWindow::backgroundColourId, porcelain::face);
    setColour (juce::Label::textColourId, porcelain::text);
    setColour (juce::PopupMenu::backgroundColourId, porcelain::face);
    setColour (juce::PopupMenu::textColourId, porcelain::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, porcelain::inset);
    setColour (juce::PopupMenu::highlightedTextColourId, porcelain::text);
    setColour (juce::ComboBox::backgroundColourId, porcelain::face);
    setColour (juce::ComboBox::textColourId, porcelain::muted);
    setColour (juce::ComboBox::outlineColourId, porcelain::line);
    setColour (juce::ComboBox::arrowColourId, porcelain::muted);
    setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    setColour (juce::TextButton::textColourOffId, porcelain::muted);
    setColour (juce::TextButton::textColourOnId, porcelain::accent);
    setColour (juce::BubbleComponent::backgroundColourId, porcelain::face);
    setColour (juce::BubbleComponent::outlineColourId, porcelain::line);
}

juce::Font PorcelainLookAndFeel::labelFont (float height) const
{
    return juce::Font (juce::FontOptions (regular).withHeight (height));
}

juce::Font PorcelainLookAndFeel::titleFont (float height) const
{
    return juce::Font (juce::FontOptions (semiBold).withHeight (height));
}

void PorcelainLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y,
                                             int width, int height, float pos,
                                             float startAngle, float endAngle,
                                             juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (5.0f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    const float radius = size * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    const bool emphasized = (bool) slider.getProperties().getWithDefault ("emphasized", false);
    const bool engaged = slider.isMouseOverOrDragging();

    g.setColour (juce::Colour (0xFFFDFCFA));                      // body
    g.fillEllipse (centre.x - radius, centre.y - radius, size, size);

    g.setColour (emphasized ? porcelain::text : porcelain::line); // ring
    g.drawEllipse (centre.x - radius, centre.y - radius, size, size,
                   emphasized ? 2.0f : 1.5f);

    if (engaged)                                                  // live-value arc
    {
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, radius + 3.0f, radius + 3.0f,
                           0.0f, startAngle, angle, true);
        g.setColour (porcelain::accent);
        g.strokePath (arc, juce::PathStrokeType (2.0f));
    }

    juce::Path pointer;                                           // notch
    pointer.addRoundedRectangle (-1.25f, -radius + 3.0f, 2.5f, radius * 0.35f, 1.0f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));
    g.setColour (porcelain::text);
    g.fillPath (pointer);
}

void PorcelainLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                                 const juce::Colour&,
                                                 bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const float corner = r.getHeight() * 0.5f;
    const bool on = b.getToggleState();

    g.setColour (on ? porcelain::accent.withAlpha (0.12f) : porcelain::face);
    g.fillRoundedRectangle (r, corner);
    g.setColour (on ? porcelain::accent
                    : (highlighted || down ? porcelain::muted : porcelain::line));
    g.drawRoundedRectangle (r, corner, 1.2f);
}

void PorcelainLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                         bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);
    g.setColour (porcelain::face);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (porcelain::line);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);

    juce::Path arrow;
    const float ax = (float) width - 14.0f, ay = (float) height * 0.5f;
    arrow.addTriangle (ax - 3.5f, ay - 2.0f, ax + 3.5f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (porcelain::muted);
    g.fillPath (arrow);
}
} // namespace vs
```

- [ ] **Step 10.3: Write `src/ui/PorcelainKnob.h`**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace vs
{
// Rotary slider with porcelain styling hooks. Its caption label is owned and
// drawn by the editor; the value readout is the drag-only popup bubble.
class PorcelainKnob : public juce::Slider
{
public:
    explicit PorcelainKnob (bool emphasized = false)
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        setPopupDisplayEnabled (true, false, nullptr);   // readout only while dragging
        getProperties().set ("emphasized", emphasized);
        setMouseDragSensitivity (180);                   // Shift-drag = fine (built in)
    }
};
} // namespace vs
```

- [ ] **Step 10.4: Add `src/ui/PorcelainLookAndFeel.cpp` to the plugin's `target_sources`, build → succeeds. Commit:**

```powershell
git add CMakeLists.txt src/ui
git commit -m "feat: Porcelain LookAndFeel (Inter, ember accent) and knob component"
```

---

### Task 11: SibilanceDisplay (hero display)

**Files:**
- Create: `src/ui/SibilanceDisplay.h`, `src/ui/SibilanceDisplay.cpp`
- Modify: `CMakeLists.txt` (add `src/ui/SibilanceDisplay.cpp` to plugin `target_sources`)

- [ ] **Step 11.1: Write `src/ui/SibilanceDisplay.h`**

```cpp
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
    juce::RangedAudioParameter* loParam = nullptr;
    juce::RangedAudioParameter* hiParam = nullptr;
};
} // namespace vs
```

- [ ] **Step 11.2: Write `src/ui/SibilanceDisplay.cpp`**

```cpp
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
```

- [ ] **Step 11.3: Add to plugin `target_sources`, build → succeeds. Commit:**

```powershell
git add CMakeLists.txt src/ui
git commit -m "feat: SibilanceDisplay - spectrum, detection tint, GR readout, range handles"
```

---

### Task 12: PresetManager + HeaderBar

**Files:**
- Create: `src/state/PresetManager.h`, `src/state/PresetManager.cpp`
- Create: `src/ui/HeaderBar.h`, `src/ui/HeaderBar.cpp`
- Modify: `CMakeLists.txt` (add both `.cpp` files to plugin `target_sources`)

- [ ] **Step 12.1: Write `src/state/PresetManager.h`**

```cpp
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
```

- [ ] **Step 12.2: Write `src/state/PresetManager.cpp`**

```cpp
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
```

- [ ] **Step 12.3: Write `src/ui/HeaderBar.h`**

```cpp
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "../PluginProcessor.h"
#include "../state/PresetManager.h"
#include "PorcelainLookAndFeel.h"

namespace vs
{
// Header: wordmark | preset menu | LISTEN pill | DARARARA | soft-bypass dot.
class HeaderBar : public juce::Component
{
public:
    HeaderBar (VocalSibilanceProcessor& proc, PorcelainLookAndFeel& lookAndFeel);

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    // Filled ember dot while processing; hollow outline while bypassed.
    class PowerDot : public juce::ToggleButton
    {
    public:
        void paintButton (juce::Graphics& g, bool, bool) override
        {
            const auto r = getLocalBounds().toFloat();
            const float d = juce::jmin (r.getWidth(), r.getHeight()) - 4.0f;
            const auto c = r.getCentre();
            if (getToggleState())   // toggle ON == bypassed
            {
                g.setColour (porcelain::line);
                g.drawEllipse (c.x - d / 2, c.y - d / 2, d, d, 1.5f);
            }
            else
            {
                g.setColour (porcelain::accent);
                g.fillEllipse (c.x - d / 2, c.y - d / 2, d, d);
            }
        }
    };

    void presetChosen();

    VocalSibilanceProcessor& processor;
    PorcelainLookAndFeel& lnf;
    PresetManager presets;

    juce::ComboBox presetBox;
    juce::TextButton listenButton { "LISTEN" };
    PowerDot bypassDot;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> listenAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
};
} // namespace vs
```

- [ ] **Step 12.4: Write `src/ui/HeaderBar.cpp`**

```cpp
#include "HeaderBar.h"

namespace vs
{
HeaderBar::HeaderBar (VocalSibilanceProcessor& proc, PorcelainLookAndFeel& lookAndFeel)
    : processor (proc), lnf (lookAndFeel), presets (proc.apvts)
{
    addAndMakeVisible (presetBox);
    presetBox.setTextWhenNothingSelected ("presets");
    int id = 1;
    for (const auto& name : presets.getFactoryNames())
        presetBox.addItem (name, id++);
    presetBox.addSeparator();
    presetBox.addItem ("Save preset...", 100);
    presetBox.addItem ("Load preset...", 101);
    presetBox.onChange = [this] { presetChosen(); };

    addAndMakeVisible (listenButton);
    listenButton.setClickingTogglesState (true);
    listenAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, "listen", listenButton);

    addAndMakeVisible (bypassDot);
    bypassDot.setTitle ("Bypass");
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, "bypass", bypassDot);
}

void HeaderBar::presetChosen()
{
    const int id = presetBox.getSelectedId();
    if (id == 0)
        return;
    if (id < 100)
    {
        presets.applyFactory (id - 1);
        return;
    }

    presetBox.setSelectedId (0, juce::dontSendNotification);   // action, not a state
    const auto dir = PresetManager::userPresetDirectory();

    if (id == 100)
    {
        chooser = std::make_unique<juce::FileChooser> ("Save preset", dir, "*.vsib");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                  | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                const auto f = fc.getResult();
                if (f != juce::File())
                    presets.saveUserPreset (f.withFileExtension ("vsib"));
            });
    }
    else if (id == 101)
    {
        chooser = std::make_unique<juce::FileChooser> ("Load preset", dir, "*.vsib");
        chooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                const auto f = fc.getResult();
                if (f.existsAsFile() && ! presets.loadUserPreset (f))
                    presetBox.setText ("preset load failed", juce::dontSendNotification);
            });
    }
}

void HeaderBar::resized()
{
    auto r = getLocalBounds();
    bypassDot.setBounds (r.removeFromRight (r.getHeight()).reduced (4));
    r.removeFromRight (juce::roundToInt (getHeight() * 2.6f));   // brand text space
    listenButton.setBounds (
        r.removeFromRight (juce::roundToInt (getHeight() * 2.2f)).reduced (0, 6));
    r.removeFromRight (8);
    presetBox.setBounds (
        r.removeFromRight (juce::roundToInt (getHeight() * 4.6f)).reduced (0, 6));
}

void HeaderBar::paint (juce::Graphics& g)
{
    const float h = (float) getHeight();

    g.setColour (porcelain::text);
    g.setFont (lnf.titleFont (h * 0.42f).withExtraKerningFactor (0.22f));
    g.drawText ("VOCAL SIBILANCE", getLocalBounds(), juce::Justification::centredLeft);

    g.setColour (porcelain::muted);
    g.setFont (lnf.labelFont (h * 0.26f).withExtraKerningFactor (0.25f));
    const auto brand = getLocalBounds()
                           .removeFromRight (getHeight() + juce::roundToInt (h * 2.6f))
                           .withTrimmedRight (getHeight() + 4);
    g.drawText ("DARARARA", brand, juce::Justification::centredRight);
}
} // namespace vs
```

- [ ] **Step 12.5: Add both `.cpp` files to plugin `target_sources`, build → succeeds. Commit:**

```powershell
git add CMakeLists.txt src/state src/ui
git commit -m "feat: PresetManager (factory + user .vsib) and HeaderBar"
```

---

### Task 13: PluginEditor — assemble the Porcelain UI

**Files:**
- Create: `src/ui/PluginEditor.h`, `src/ui/PluginEditor.cpp`
- Modify: `CMakeLists.txt` (add `src/ui/PluginEditor.cpp` to plugin `target_sources`; DELETE the line `target_compile_definitions(VocalSibilance PRIVATE VS_USE_GENERIC_EDITOR=1)` — keep the dsp_tests one)

- [ ] **Step 13.1: Write `src/ui/PluginEditor.h`**

```cpp
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "../PluginProcessor.h"
#include "PorcelainLookAndFeel.h"
#include "PorcelainKnob.h"
#include "SibilanceDisplay.h"
#include "HeaderBar.h"

class VocalSibilanceEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalSibilanceEditor (VocalSibilanceProcessor&);
    ~VocalSibilanceEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    VocalSibilanceProcessor& processor;
    vs::PorcelainLookAndFeel lnf;

    vs::HeaderBar header;
    vs::SibilanceDisplay display;

    vs::PorcelainKnob smoothKnob { true };   // emphasized
    vs::PorcelainKnob gritKnob, characterKnob, mixKnob, outputKnob;
    juce::Label smoothLabel, gritLabel, characterLabel, mixLabel, outputLabel;

    std::unique_ptr<SliderAttachment> smoothAtt, gritAtt, characterAtt, mixAtt, outputAtt;

    static constexpr int baseWidth = 520, baseHeight = 360;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalSibilanceEditor)
};
```

- [ ] **Step 13.2: Write `src/ui/PluginEditor.cpp`**

```cpp
#include "PluginEditor.h"

VocalSibilanceEditor::VocalSibilanceEditor (VocalSibilanceProcessor& p)
    : AudioProcessorEditor (p), processor (p), header (p, lnf), display (p)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (header);
    addAndMakeVisible (display);

    struct KnobSpec
    {
        vs::PorcelainKnob& knob;
        juce::Label& label;
        const char* id;
        const char* text;
        double resetTo;
    };
    const KnobSpec specs[] = {
        { smoothKnob,    smoothLabel,    "smooth",    "SMOOTH",     35.0 },
        { gritKnob,      gritLabel,      "grit",      "GRIT",        0.0 },
        { characterKnob, characterLabel, "character", "CHARACTER",  30.0 },
        { mixKnob,       mixLabel,       "mix",       "MIX",       100.0 },
        { outputKnob,    outputLabel,    "output",    "OUTPUT",      0.0 },
    };
    for (const auto& s : specs)
    {
        addAndMakeVisible (s.knob);
        s.knob.setDoubleClickReturnValue (true, s.resetTo);
        s.knob.setTitle (s.text);
        addAndMakeVisible (s.label);
        s.label.setText (s.text, juce::dontSendNotification);
        s.label.setJustificationType (juce::Justification::centred);
        s.label.setColour (juce::Label::textColourId, vs::porcelain::muted);
        s.label.setInterceptsMouseClicks (false, false);
    }

    smoothAtt    = std::make_unique<SliderAttachment> (p.apvts, "smooth",    smoothKnob);
    gritAtt      = std::make_unique<SliderAttachment> (p.apvts, "grit",      gritKnob);
    characterAtt = std::make_unique<SliderAttachment> (p.apvts, "character", characterKnob);
    mixAtt       = std::make_unique<SliderAttachment> (p.apvts, "mix",       mixKnob);
    outputAtt    = std::make_unique<SliderAttachment> (p.apvts, "output",    outputKnob);

    setResizable (true, true);
    setResizeLimits (312, 216, 1040, 720);                 // 60 % .. 200 %
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) baseWidth / baseHeight);

    const double scale = processor.apvts.state.getProperty ("uiScale", 1.0);
    setSize (juce::roundToInt (baseWidth * scale), juce::roundToInt (baseHeight * scale));
}

VocalSibilanceEditor::~VocalSibilanceEditor()
{
    setLookAndFeel (nullptr);
}

void VocalSibilanceEditor::paint (juce::Graphics& g)
{
    g.fillAll (vs::porcelain::face);
}

void VocalSibilanceEditor::resized()
{
    processor.apvts.state.setProperty ("uiScale", (double) getWidth() / baseWidth, nullptr);

    const float s = (float) getWidth() / baseWidth;
    auto r = getLocalBounds().reduced (juce::roundToInt (14 * s));

    header.setBounds (r.removeFromTop (juce::roundToInt (34 * s)));
    r.removeFromTop (juce::roundToInt (10 * s));
    display.setBounds (r.removeFromTop (juce::roundToInt (140 * s)));
    r.removeFromTop (juce::roundToInt (12 * s));

    juce::Label* labels[] = { &smoothLabel, &gritLabel, &characterLabel, &mixLabel, &outputLabel };
    vs::PorcelainKnob* knobs[] = { &smoothKnob, &gritKnob, &characterKnob, &mixKnob, &outputKnob };
    const int colW = r.getWidth() / 5;
    for (int i = 0; i < 5; ++i)
    {
        auto col = r.withX (r.getX() + i * colW).withWidth (colW);
        auto labelArea = col.removeFromBottom (juce::roundToInt (16 * s));
        labels[i]->setBounds (labelArea);
        labels[i]->setFont (juce::Font (juce::FontOptions (10.0f * s))
                                .withExtraKerningFactor (0.15f));
        const int kSize = juce::jmin (col.getWidth(), col.getHeight())
                          - juce::roundToInt (8 * s);
        knobs[i]->setBounds (col.withSizeKeepingCentre (kSize, kSize));
    }
}
```

- [ ] **Step 13.3: CMake swap, full rebuild**

Add `src/ui/PluginEditor.cpp` to plugin `target_sources`. Delete the line `target_compile_definitions(VocalSibilance PRIVATE VS_USE_GENERIC_EDITOR=1)`. (The dsp_tests definition stays.)

Run: `cmake -B build` then `cmake --build build --config Release --parallel` then `ctest --test-dir build -C Release --output-on-failure`
Expected: build succeeds, all tests still pass.

- [ ] **Step 13.4: Visual verification (Standalone)**

Launch the Standalone exe and verify each item:
1. Porcelain face, Inter wordmark "VOCAL SIBILANCE", "DARARARA" + ember dot top-right.
2. Spectrum animates with mic/loopback input; bars inside the range turn ember on "s" sounds.
3. Range handles drag with kHz readout; can't cross within half an octave.
4. SMOOTH knob has the strong dark ring; dragging any knob shows the value bubble and ember arc.
5. Double-click resets a knob; Shift-drag is fine-grained; mouse-wheel nudges.
6. LISTEN pill solos the band and dims the out-of-range spectrum.
7. Presets menu applies each factory preset; Save/Load round-trips a `.vsib` file.
8. Window resizes 60–200 % with crisp vector redraw; size is remembered after close/reopen.
9. Power dot toggles soft bypass without clicks.

- [ ] **Step 13.5: Commit**

```powershell
git add CMakeLists.txt src/ui
git commit -m "feat: Porcelain Display-First editor - resizable, scale persisted"
```

---

### Task 14: README, LICENSE, gitignore

**Files:**
- Create: `LICENSE` (downloaded AGPLv3 text)
- Create: `README.md`
- Modify: `.gitignore`

- [ ] **Step 14.1: Download the license text**

```powershell
curl.exe -L -o LICENSE https://www.gnu.org/licenses/agpl-3.0.txt
```

Verify: first line of `LICENSE` reads `GNU AFFERO GENERAL PUBLIC LICENSE`.

- [ ] **Step 14.2: Write `README.md`**

```markdown
# Vocal Sibilance — by dararara

A free, open-source de-esser for vocals. **Smooth** tames harsh "s" sounds
without dulling the voice. **Grit** is the twist: a detector-gated saturator
that rides only the sibilant moments, turning harshness into texture instead
of just removing it. The **Character** knob morphs the grit from warm tape
rounding to edgy fuzz.

## Formats & DAWs

VST3 · AU · CLAP — Windows 10+ and macOS 10.13+ (Intel & Apple Silicon).
Works in FL Studio, Ableton Live, Cubase, Studio One, Reaper, Logic Pro,
GarageBand, Bitwig and any other VST3/AU/CLAP host. (AAX / Pro Tools: planned.)

## Install

Grab the latest installer from the
[Releases page](../../releases). The Windows installer copies the plugins to
`C:\Program Files\Common Files\{VST3,CLAP}`. The macOS package installs to
`/Library/Audio/Plug-Ins/{VST3,Components,CLAP}`.

### macOS: "cannot be opened" warning

These builds are not Apple-notarized (this is a free tool — no Apple developer
subscription). After installing, clear the quarantine flag once:

    sudo xattr -cr "/Library/Audio/Plug-Ins/VST3/Vocal Sibilance.vst3"
    sudo xattr -cr "/Library/Audio/Plug-Ins/Components/Vocal Sibilance.component"
    sudo xattr -cr "/Library/Audio/Plug-Ins/CLAP/Vocal Sibilance.clap"

## Controls

| Control | What it does |
|---|---|
| Display | Live spectrum; ember = detected sibilance; drag the two handles to set the range |
| SMOOTH | How much sibilance reduction (up to 15 dB, threshold-free detection) |
| GRIT | Saturation amount, applied only while sibilance is detected |
| CHARACTER | Grit flavor: warm tape (left) to edgy fuzz (right) |
| MIX | Parallel dry/wet |
| OUTPUT | Output trim ±12 dB |
| LISTEN | Solo what the plugin is acting on |

## Build from source

    cmake -B build
    cmake --build build --config Release --parallel
    ctest --test-dir build -C Release

Requires CMake ≥ 3.22 and a C++20 compiler. JUCE and all dependencies are
fetched automatically.

## License

AGPLv3 — see [LICENSE](LICENSE). Inter font © The Inter Project Authors,
SIL OFL 1.1 (`assets/fonts/InterLicense-OFL.txt`).
```

- [ ] **Step 14.3: Extend `.gitignore`** so it contains exactly:

```gitignore
.superpowers/
build/
*.zip
inter_tmp/
packaging/Output/
*.pkg
```

- [ ] **Step 14.4: Commit**

```powershell
git add LICENSE README.md .gitignore
git commit -m "docs: README, AGPLv3 license, gitignore"
```

---

### Task 15: CI — build, test, pluginval on every push

**Files:**
- Create: `.github/workflows/build.yml`

- [ ] **Step 15.1: Write `.github/workflows/build.yml`**

```yaml
name: Build
on:
  push:
    branches: ["**"]
  pull_request:

jobs:
  build:
    strategy:
      fail-fast: false
      matrix:
        include:
          - os: windows-latest
            name: Windows
          - os: macos-latest
            name: macOS
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4

      - name: Configure (Windows)
        if: runner.os == 'Windows'
        run: cmake -B build

      - name: Configure (macOS universal)
        if: runner.os == 'macOS'
        run: >
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"
          -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13

      - name: Build
        run: cmake --build build --config Release --parallel

      - name: Test
        run: ctest --test-dir build -C Release --output-on-failure

      - name: Get pluginval (Windows)
        if: runner.os == 'Windows'
        run: |
          curl -L -o pluginval.zip https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_Windows.zip
          7z x pluginval.zip

      - name: Get pluginval (macOS)
        if: runner.os == 'macOS'
        run: |
          curl -L -o pluginval.zip https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_macOS.zip
          unzip pluginval.zip

      - name: pluginval (Windows)
        if: runner.os == 'Windows'
        run: ./pluginval.exe --strictness-level 10 --skip-gui-tests --validate "build/VocalSibilance_artefacts/Release/VST3/Vocal Sibilance.vst3"

      - name: pluginval (macOS)
        if: runner.os == 'macOS'
        run: ./pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --skip-gui-tests --validate "build/VocalSibilance_artefacts/Release/VST3/Vocal Sibilance.vst3"

      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: vocal-sibilance-${{ matrix.name }}
          path: |
            build/VocalSibilance_artefacts/Release/VST3
            build/VocalSibilance_artefacts/Release/CLAP
            build/VocalSibilance_artefacts/Release/AU
            build/VocalSibilance_artefacts/Release/Standalone
```

- [ ] **Step 15.2: Commit, create the GitHub repo, push**

**CHECKPOINT — needs the user:** ask for (or create with their approval) the GitHub repository (e.g. `dararara/vocal-sibilance`, public, since the project is AGPLv3). Then:

```powershell
git add .github
git commit -m "ci: build, test, pluginval on Windows and macOS"
git remote add origin https://github.com/<OWNER>/vocal-sibilance.git
git push -u origin master
```

- [ ] **Step 15.3: Verify CI**

Open the repo's Actions tab. Expected: both Windows and macOS jobs green — this is the first time the macOS build (AU included) is proven.

---

### Task 16: Packaging — installers and tagged releases

**Files:**
- Create: `packaging/windows-installer.iss`
- Create: `packaging/make-macos-pkg.sh`
- Create: `.github/workflows/release.yml`

- [ ] **Step 16.1: Write `packaging/windows-installer.iss`**

```ini
#ifndef MyAppVersion
  #define MyAppVersion "1.0.0"
#endif

[Setup]
AppName=Vocal Sibilance
AppVersion={#MyAppVersion}
AppPublisher=dararara
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputBaseFilename=VocalSibilance-{#MyAppVersion}-Windows
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Files]
Source: "..\build\VocalSibilance_artefacts\Release\VST3\Vocal Sibilance.vst3\*"; DestDir: "{commoncf64}\VST3\Vocal Sibilance.vst3"; Flags: ignoreversion recursesubdirs
Source: "..\build\VocalSibilance_artefacts\Release\CLAP\Vocal Sibilance.clap"; DestDir: "{commoncf64}\CLAP"; Flags: ignoreversion
```

- [ ] **Step 16.2: Write `packaging/make-macos-pkg.sh`**

```bash
#!/usr/bin/env bash
# Builds an unsigned macOS .pkg installing VST3 + AU + CLAP. Run from repo root.
set -euo pipefail
VERSION="${1:-1.0.0}"
ART="build/VocalSibilance_artefacts/Release"
STAGE="$(mktemp -d)"

mkdir -p "$STAGE/VST3" "$STAGE/Components" "$STAGE/CLAP"
cp -R "$ART/VST3/Vocal Sibilance.vst3"      "$STAGE/VST3/"
cp -R "$ART/AU/Vocal Sibilance.component"   "$STAGE/Components/"
cp -R "$ART/CLAP/Vocal Sibilance.clap"      "$STAGE/CLAP/"

pkgbuild --root "$STAGE/VST3" --identifier com.dararara.vocalsibilance.vst3 \
         --version "$VERSION" --install-location "/Library/Audio/Plug-Ins/VST3" vst3.pkg
pkgbuild --root "$STAGE/Components" --identifier com.dararara.vocalsibilance.au \
         --version "$VERSION" --install-location "/Library/Audio/Plug-Ins/Components" au.pkg
pkgbuild --root "$STAGE/CLAP" --identifier com.dararara.vocalsibilance.clap \
         --version "$VERSION" --install-location "/Library/Audio/Plug-Ins/CLAP" clap.pkg

productbuild --package vst3.pkg --package au.pkg --package clap.pkg \
             "VocalSibilance-$VERSION-macOS.pkg"
rm -f vst3.pkg au.pkg clap.pkg
rm -rf "$STAGE"
echo "Created VocalSibilance-$VERSION-macOS.pkg (unsigned)"
```

- [ ] **Step 16.3: Write `.github/workflows/release.yml`**

```yaml
name: Release
on:
  push:
    tags: ['v*']

permissions:
  contents: write

jobs:
  release:
    strategy:
      fail-fast: false
      matrix:
        include:
          - os: windows-latest
          - os: macos-latest
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4

      - name: Configure (Windows)
        if: runner.os == 'Windows'
        run: cmake -B build

      - name: Configure (macOS universal)
        if: runner.os == 'macOS'
        run: >
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"
          -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13

      - name: Build
        run: cmake --build build --config Release --parallel

      - name: Test
        run: ctest --test-dir build -C Release --output-on-failure

      - name: Package (Windows)
        if: runner.os == 'Windows'
        shell: pwsh
        run: |
          $v = $env:GITHUB_REF_NAME.TrimStart('v')
          & "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe" /DMyAppVersion="$v" packaging\windows-installer.iss

      - name: Package (macOS)
        if: runner.os == 'macOS'
        run: bash packaging/make-macos-pkg.sh "${GITHUB_REF_NAME#v}"

      - name: Publish release
        uses: softprops/action-gh-release@v2
        with:
          files: |
            packaging/Output/*.exe
            VocalSibilance-*-macOS.pkg
```

- [ ] **Step 16.4: Commit and push**

```powershell
git add packaging .github
git commit -m "feat: Windows/macOS installers and tagged-release workflow"
git push
```

---

### Task 17: Final verification (release gate)

No code changes — this task proves the success criteria in the spec before tagging v1.0.0.

- [ ] **Step 17.1: Local pluginval at full strictness (GUI tests included)**

```powershell
curl.exe -L -o pluginval.zip https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_Windows.zip
Expand-Archive pluginval.zip -DestinationPath pluginval_tmp
& "pluginval_tmp\pluginval.exe" --strictness-level 10 --validate "build\VocalSibilance_artefacts\Release\VST3\Vocal Sibilance.vst3"
Remove-Item -Recurse -Force pluginval_tmp, pluginval.zip
```

Expected: `ALL TESTS PASSED`.

- [ ] **Step 17.2: Windows DAW matrix (manual, with the user)**

For each available DAW (FL Studio at minimum; Reaper's free eval covers a second host; Ableton/Cubase if installed): load the VST3 on a vocal track, verify audio passes; automate SMOOTH; toggle LISTEN and bypass; save and reopen the project and confirm all settings + window size restore; check CLAP in Reaper or Bitwig if available.

- [ ] **Step 17.3: macOS check (via CI artifacts)**

Download the macOS artifact from CI and have a Mac user (or a borrowed machine) confirm: `xattr -cr` workaround works, AU passes Logic's plugin validation (auval), settings persist.

- [ ] **Step 17.4: Ship v1.0.0 (CHECKPOINT — user approves)**

```powershell
git tag v1.0.0
git push origin v1.0.0
```

Expected: Release workflow publishes `VocalSibilance-1.0.0-Windows.exe` and `VocalSibilance-1.0.0-macOS.pkg` on the GitHub Releases page.

---

## Definition of Done (mirrors spec §10)

1. All Catch2 suites green; pluginval strictness 10 passes on Windows + macOS CI.
2. Neutral-settings null and bypass null proven by tests.
3. Aliasing test enforces the 40 dB floor.
4. Plugin verified in the Windows DAW matrix; macOS artifact verified by a Mac tester.
5. Porcelain UI matches the approved mockup direction at every scale 60–200 %.
6. v1.0.0 release published with both installers.
