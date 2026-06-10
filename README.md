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
