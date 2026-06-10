#ifndef MyAppVersion
  #define MyAppVersion "1.0.0"
#endif

[Setup]
AppName=Vocal Sibilance
AppVersion={#MyAppVersion}
AppPublisher=bydararara
AppPublisherURL=https://www.youtube.com/@darararabeats
AppSupportURL=https://github.com/leevmors/vocal-sibilance
AppCopyright=Copyright (C) 2026 bydararara
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableWelcomePage=no
WizardStyle=modern
WizardImageFile=wizard-large.bmp
WizardSmallImageFile=wizard-small.bmp
InfoBeforeFile=installer-info.txt
LicenseFile=..\LICENSE
OutputBaseFilename=VocalSibilance-{#MyAppVersion}-Windows
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64
UninstallDisplayName=Vocal Sibilance (bydararara)
VersionInfoVersion={#MyAppVersion}
VersionInfoCompany=bydararara
VersionInfoDescription=Vocal Sibilance de-esser installer

[Messages]
WelcomeLabel1=Welcome to Vocal Sibilance
WelcomeLabel2=This will install [name/ver] by bydararara on your computer.%n%nVocal Sibilance is a free de-esser that smooths harsh "s" sounds - or turns them into texture with its detector-gated Grit engine.%n%nPlease close your DAW before continuing.

[Types]
Name: "full"; Description: "Full installation"
Name: "compact"; Description: "VST3 only"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin (recommended)"; Types: full compact custom; Flags: fixed
Name: "clap"; Description: "CLAP plugin (for Bitwig, Reaper and other CLAP hosts)"; Types: full

[Files]
Source: "..\build\VocalSibilance_artefacts\Release\VST3\Vocal Sibilance.vst3\*"; DestDir: "{commoncf64}\VST3\Vocal Sibilance.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs
Source: "..\LICENSE"; DestDir: "{commoncf64}\VST3\Vocal Sibilance.vst3"; DestName: "LICENSE.txt"; Components: vst3; Flags: ignoreversion
Source: "..\assets\fonts\InterLicense-OFL.txt"; DestDir: "{commoncf64}\VST3\Vocal Sibilance.vst3"; Components: vst3; Flags: ignoreversion
Source: "..\build\VocalSibilance_artefacts\Release\CLAP\Vocal Sibilance.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap; Flags: ignoreversion
