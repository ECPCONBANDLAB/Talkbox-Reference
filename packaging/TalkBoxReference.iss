; TalkBox Reference — Windows Inno Setup script
; Registers cleanly in Installed Apps. No telemetry, no services.
;
; Built by .github/workflows/windows-installer.yml on a windows-latest runner,
; which is where this script is compiled (Inno Setup 6 is preinstalled there).
; To build locally, build the plugin first:
;
;   cmake -B build -DCMAKE_BUILD_TYPE=Release -DTB_BUILD_JUCE=ON -DJUCE_PATH=<juce-7.0.12>
;   cmake --build build --config Release
;
; then run:  ISCC packaging\TalkBoxReference.iss
;
; Paths below match JUCE 7.0.12's artefact layout:
;   build/TalkBoxReference_artefacts/Release/{VST3,Standalone}/...
;
; No CLAP entry: JUCE 7.0.12 has no CLAP wrapper (see docs/07).
;
; LICENCE: this binary links JUCE 7 under the GPLv3. See LICENSE-NOTES.md.

#define MyAppName "TalkBox Reference"
#define MyAppVersion "1.3.0"
#define MyAppPublisher "TalkBox Reference"
; Paths in this script are relative to this file (packaging\), so the
; repo-root items are reached with ..\
#define BuildDir "..\build\TalkBoxReference_artefacts\Release"

[Setup]
AppId={{8F3A2B1C-4D5E-4F6A-9B8C-7D6E5F4A3B2C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\TalkBox Reference
DefaultGroupName=TalkBox Reference
OutputDir=..\dist
OutputBaseFilename=TalkBoxReference-Setup-{#MyAppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
LicenseFile=..\COPYING-GPL-3.0.txt
ArchitecturesInstallIn64BitMode=x64compatible
; Let the user choose what to install. Both are selected by default because
; most people want both, but a user who only mixes inside a DAW can skip the
; standalone app, and vice versa.
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full";       Description: "Everything (VST3 for your DAW + Standalone app)"
Name: "vst3only";   Description: "VST3 plugin only (for your DAW)"
Name: "standalone"; Description: "Standalone app only"
Name: "custom";     Description: "Custom selection"; Flags: iscustom

[Components]
Name: "vst3";       Description: "VST3 plugin (for FL Studio, Ableton, Reaper, Cubase...)"; Types: full vst3only
Name: "standalone"; Description: "Standalone app (play without a DAW)";                 Types: full standalone

[Files]
; VST3 bundle -> the shared VST3 folder so every DAW sees it
Source: "{#BuildDir}\VST3\TalkBoxReference.vst3\*"; DestDir: "{autopf}\Common Files\VST3\TalkBoxReference.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
; Standalone app + its dependencies, into its own program folder
Source: "{#BuildDir}\Standalone\*"; DestDir: "{autopf}\{#MyAppName}"; Components: standalone; Flags: ignoreversion recursesubdirs createallsubdirs
; Licence files go with whichever component is installed
Source: "..\COPYING-GPL-3.0.txt"; DestDir: "{autopf}\{#MyAppName}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{autopf}\{#MyAppName}"; DestName: "LICENSE-MIT.txt"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{autopf}\{#MyAppName}\TalkBoxReference.exe"; Components: standalone

[Run]
Filename: "{autopf}\{#MyAppName}\TalkBoxReference.exe"; Description: "Launch {#MyAppName}"; Components: standalone; Flags: nowait postinstall skipifsilent

[UninstallRun]
; Clean uninstaller prompts Keep presets vs Remove all (see docs/07)
