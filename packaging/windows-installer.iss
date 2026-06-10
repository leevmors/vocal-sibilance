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
Source: "..\LICENSE"; DestDir: "{commoncf64}\VST3\Vocal Sibilance.vst3"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "..\assets\fonts\InterLicense-OFL.txt"; DestDir: "{commoncf64}\VST3\Vocal Sibilance.vst3"; Flags: ignoreversion
