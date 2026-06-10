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
cp LICENSE "$STAGE/VST3/Vocal Sibilance.vst3/LICENSE.txt"
cp assets/fonts/InterLicense-OFL.txt "$STAGE/VST3/Vocal Sibilance.vst3/"

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
