; TalkBox Reference — Windows Inno Setup script
; Registers cleanly in Installed Apps. No telemetry, no services.
;
; NOT built by CI and NOT shipped in v1.0.0. This script only packages the
; output of a Release build, so build the plugin first:
;
;   cmake -B build -DCMAKE_BUILD_TYPE=Release -DTB_BUILD_JUCE=ON -DJUCE_PATH=<juce-7.0.12>
;   cmake --build build --config Release
;
; The paths below match JUCE 7.0.12's artefact layout:
;   build/TalkBoxReference_artefacts/Release/<Format>/...
;
; No CLAP entry: JUCE 7.0.12 has no CLAP wrapper (see docs/07).

#define MyAppName "TalkBox Reference"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "TalkBox Reference"
#define MyAppExeName "TalkBox Reference.exe"
#define BuildDir "build\TalkBoxReference_artefacts\Release"

[Setup]
AppId={{8F3A2B1C-4D5E-4F6A-9B8C-7D6E5F4A3B2C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\TalkBox Reference
DefaultGroupName=TalkBox Reference
DisableProgramIcons=yes
OutputBaseFilename=TalkBoxReference-Setup-{#MyAppVersion}
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#BuildDir}\VST3\TalkBox Reference.vst3\*"; DestDir: "{autopf}\Common Files\VST3\TalkBox Reference.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\*"; DestDir: "{autopf}\TalkBox Reference"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "presets\factory_presets.json"; DestDir: "{autopf}\TalkBox Reference\presets"; Flags: ignoreversion

[Icons]
Name: "{group}\TalkBox Reference"; Filename: "{autopf}\TalkBox Reference\{#MyAppExeName}"

[UninstallRun]
; Clean uninstaller prompts Keep presets vs Remove all (see docs/07)
