# Vocal Sibilance — Design Specification

**Date:** 2026-06-10
**Product:** Vocal Sibilance by dararara
**Status:** Approved design, pending implementation plan

## 1. Product Overview

Vocal Sibilance is a free, open-source de-esser plugin for vocals. It does two
things:

1. **Smooth** — tames harsh sibilance ("ess", "sh", "t" sounds) so vocals stop
   hurting the listener's ears, without dulling the rest of the voice.
2. **Grit** — optionally saturates the sibilant band with a morphable character,
   turning harshness into pleasing, juicy texture instead of merely removing it.

The grit stage is the differentiator: where ordinary de-essers only subtract,
Vocal Sibilance can replace harshness with character.

**Brand:** UI shows "VOCAL SIBILANCE" (left) and "DARARARA" (right).
**Price:** Free. **License:** Open source, AGPLv3 (keeps JUCE free, no splash screen).

## 2. Scope

### v1.0 (this design)

- **Platforms:** Windows 10+ (x64), macOS 10.13+ (universal x64/ARM)
- **Formats:** VST3, AU, CLAP, plus a Standalone app for development testing
- **DAW coverage:** FL Studio, Ableton Live, Cubase, Studio One, Reaper,
  Logic Pro (AU), GarageBand, Bitwig (CLAP), and any other VST3/AU/CLAP host
- **Channel layouts:** mono and stereo

### Explicitly out of scope for v1.0

- **AAX / Pro Tools** — added in a later release once the free Avid developer
  agreement and PACE signing are sorted. The JUCE codebase already supports the
  AAX target, so this is a release-engineering task, not a redesign.
- **macOS code signing / notarization** — decided against paying for an Apple
  Developer ID ($99/year). Mac builds ship unsigned; the download page and
  README document the standard workaround (right-click → Open, or
  `xattr -cr` on the plugin bundle). Revisit only if Mac-user friction proves
  significant.
- Advanced controls drawer (attack/release, lookahead, oversampling toggles)
- A/B compare, auto-update mechanisms

## 3. Controls (8 parameters, all DAW-automatable)

| Parameter | Range | Default | Purpose |
|---|---|---|---|
| Smooth | 0–100 % | 35 % | Maximum sibilance reduction depth (0 % = bit-transparent) |
| Grit | 0–100 % | 0 % | Saturation amount applied to the detected sibilant band |
| Character | 0–100 % | 30 % | Morphs grit curve: warm tape rounding → edgy asymmetric fuzz |
| Mix | 0–100 % | 100 % | Global dry/wet (parallel processing) |
| Output | −12…+12 dB | 0 dB | Output trim |
| Range Lo | 2–16 kHz | 4 kHz | Lower bound of detection/processing band |
| Range Hi | 2–16 kHz | 11 kHz | Upper bound of detection/processing band (always > Range Lo) |
| Listen | on/off | off | Solo the processed sibilant band |

Plus a soft-bypass power toggle (crossfaded, click-free) in the header.

## 4. DSP Design (the engine)

Signal flow per channel:

```
in ──► Linkwitz-Riley split ──► low/rest ────────────────────────────┐
            │                                                        ▼
            └─► sibilant band ─► gain reduction ─► grit shaper ──► sum ─► mix ─► output
                     │                ▲                 ▲
                     └─► detector ────┴─────────────────┘  (drive follows detector)
```

1. **Band isolation:** 4th-order Linkwitz-Riley crossovers isolate the
   Range Lo–Range Hi band. LR4 sums back flat, so untouched audio is
   untouched. Only this band is ever processed — no wideband ducking/lisping.
2. **Detection:** Sidechain measures sibilant-band energy *relative to*
   full-band vocal energy (rolling comparison). Relative detection keeps
   behavior consistent across quiet and loud passages — no threshold knob.
   Envelope follower: near-instant attack (≈1 ms), program-dependent release
   (≈30–80 ms). Stereo detection is linked (max of both channels) so the
   stereo image never wobbles.
3. **Reduction:** Soft-knee gain computer driven by the detector. Smooth maps
   0–100 % to a 0–15 dB maximum reduction depth. Gain changes are smoothed —
   no zipper noise, no clicks.
4. **Grit:** Waveshaper on the sibilant band, post-reduction. Drive is scaled
   by the detector envelope, so saturation rides only the ess sounds — never
   breath or vocal body. Character morphs the transfer curve continuously:
   0 % = soft symmetric tanh (tape-like), 100 % = harder asymmetric curve with
   richer upper harmonics. The stage runs **4× oversampled** with proper
   anti-aliasing filters; aliasing in the 5–15 kHz region is audible garbage
   and defeats the product's purpose.
5. **Recombine → Mix → Output.** Listen mode taps the processed sibilant band
   and routes it alone to the output.

### Engine quality invariants

- Zero added latency (IIR crossovers, no lookahead) — safe while tracking.
  Oversampling latency inside the grit stage is compensated internally.
- With Smooth = 0 and Grit = 0, output nulls against input (bit-transparent),
  proven by an automated test.
- No allocation, locking, or system calls on the audio thread.
- Denormal protection (FTZ/DAZ); NaN/Inf scrubbing at the output so the plugin
  can never spray garbage into a mix.
- Sample-rate agnostic: 44.1–192 kHz. Handles any buffer size, including
  1-sample and varying sizes.

## 5. Architecture

**Stack:** JUCE 8 (C++20), CMake, clap-juce-extensions for the CLAP target.

```
vocal-sibilance/
├── CMakeLists.txt
├── src/
│   ├── dsp/                  # pure DSP, no JUCE-GUI deps, unit-testable
│   │   ├── CrossoverBands.{h,cpp}
│   │   ├── SibilanceDetector.{h,cpp}
│   │   ├── GainComputer.{h,cpp}
│   │   ├── GritShaper.{h,cpp}
│   │   └── SibilanceEngine.{h,cpp}   # wires the chain, owns channels
│   ├── PluginProcessor.{h,cpp}      # APVTS params, state, engine hookup
│   └── ui/
│       ├── PluginEditor.{h,cpp}
│       ├── PorcelainLookAndFeel.{h,cpp}
│       ├── SibilanceDisplay.{h,cpp} # spectrum + range handles + GR overlay
│       ├── PorcelainKnob.{h,cpp}
│       └── HeaderBar.{h,cpp}        # title, presets, bypass, brand
├── tests/                            # DSP unit tests (Catch2 or JUCE UnitTest)
├── presets/                          # factory presets (JSON/XML)
└── .github/workflows/                # CI: build, test, pluginval, package
```

- **Parameters** via `AudioProcessorValueTreeState`; state serialized into the
  DAW session with a version field so future releases load old sessions.
- **Audio→UI data** (spectrum, detector state, gain reduction) flows through a
  lock-free FIFO; the editor polls on a 30 Hz timer. The audio thread never
  waits on the UI.
- Each `dsp/` unit has one job and a documented interface; all are testable
  without instantiating a plugin host.

## 6. UI Design

**Style: "Porcelain"** (user-selected from four directions) — warm off-white
face `#F7F6F3`, panel inset `#EFEDE8`, hairlines `#DCD9D2`, near-black text
`#2A2A28`, muted text `#6B6862`, single ember accent `#E0552C` reserved for
live information (active sibilance, range band, value being edited). Bundled
Inter font; letter-spaced uppercase labels.

**Layout: "Display First"** (user-selected from three layouts):

- **Header:** VOCAL SIBILANCE wordmark · slim preset menu · DARARARA · soft-bypass dot
- **Hero display:** live input spectrum (~30 fps). Bars inside the detection
  range light ember when the detector fires; an overlay shows current gain
  reduction so the plugin visibly explains itself. Two draggable range handles
  with frequency readout while dragging; the active band is gently tinted.
  While Listen is on, everything outside the range dims.
- **Knob row:** SMOOTH (visually emphasized: stronger ring) · GRIT · CHARACTER
  · MIX · OUTPUT.

**Interaction:** drag to turn, Shift-drag = fine, double-click = reset,
mouse-wheel supported, value readout appears only while interacting. Full JUCE
accessibility (screen-reader names, keyboard navigation).

**Window:** freely resizable 60–200 % (all vector drawing), default ≈520×360,
scale persisted in plugin state.

**Presets:** six factory presets — Gentle, Standard, Bright Vocal, Tame Harsh
Mic, Gritty Pop, Maximum Character — plus user save/load to disk
(cross-platform user-presets folder). DAW session state remains the primary
persistence as normal.

## 7. Error Handling & Edge Cases

- Unusual hosts: any buffer size (1…8192+, varying), sample-rate changes at
  runtime, bypass toggling mid-note (crossfaded), automation jumps (smoothed).
- Mono↔stereo and channel-layout negotiation handled in `prepareToPlay`.
- Range handles constrained so Lo < Hi with a minimum gap (~0.5 octave).
- State loading tolerates missing/unknown fields (forward compatibility).
- Preset file I/O failures are non-fatal and reported in the UI unobtrusively.

## 8. Testing & Release Engineering

**Automated (runs in CI on every push):**

- DSP unit tests: LR4 crossover sums flat (null), neutral-settings null against
  dry, detector fires on synthesized ess bursts and stays quiet on vowels,
  gain computer curve correctness, measured aliasing of the grit stage below
  agreed threshold with oversampling on.
- pluginval at maximum strictness against the VST3 build (Windows and macOS).

**Manual before each release:** DAW matrix — FL Studio, Ableton, Cubase,
Reaper (Windows, local); Logic Pro / GarageBand (macOS, CI artifacts on a test
Mac or tester volunteers). CLAP smoke-tested in Bitwig or Reaper.

**CI/CD:** GitHub Actions, Windows + macOS runners. Pipeline: configure →
build all formats → run tests → pluginval → package. Artifacts: Windows Inno
Setup installer (copies VST3/CLAP to standard folders) and macOS .pkg
(installs VST3/AU/CLAP to `/Library/Audio/Plug-Ins/...`), both unsigned.

**Distribution:** GitHub Releases (semver). Source on GitHub under AGPLv3.
Download page/README includes the macOS unsigned-plugin instructions
(right-click → Open / `xattr -cr`). Landing page is a possible follow-up, not
part of v1.0.

## 9. Decision Log

| Decision | Choice | Why |
|---|---|---|
| v1.0 targets | Win+Mac, VST3+AU+CLAP | Covers all named DAWs except Pro Tools without signing bureaucracy |
| AAX | Deferred post-1.0 | Requires Avid agreement + PACE signing; JUCE keeps it cheap later |
| Control philosophy | Minimal-but-complete (8 params) | Beautiful minimalism without being underpowered |
| Grit design | Detector-gated band saturation, morphing Character knob | Differentiator; grit rides only the ess sounds; no mode menus |
| Visual style | Porcelain (off-white, ember accent) | User-selected from 4 mockups |
| Layout | Display First | User-selected from 3 mockups; "see what it's catching" |
| Framework | JUCE 8, native vector UI | One codebase → all formats; free CI; lightweight; proven |
| License | AGPLv3 open source | Keeps JUCE free, no splash; fits free-tool spirit |
| macOS signing | None (unsigned, documented workaround) | User decision: no $99/year Apple Developer ID |
| Latency | Zero | Tracking-safe; fast attack suffices for v1 |

## 10. Success Criteria

1. Loads and passes audio correctly in FL Studio, Ableton, Cubase, Reaper,
   Logic Pro, and Bitwig.
2. Neutral settings are bit-transparent (automated null test proves it).
3. Sibilance reduction is audibly transparent on real vocal material — no
   lisping, no dulled air band.
4. Grit adds harmonics only to sibilant moments, with no audible aliasing at
   any Character/Grit setting (measured, not vibes).
5. pluginval max-strictness passes on both platforms.
6. UI renders crisply at all scales; a first-time user needs no manual.
