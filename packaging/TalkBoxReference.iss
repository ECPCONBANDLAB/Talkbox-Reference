; TalkBox Reference — Windows Inno Setup script
; Registers cleanly in Installed Apps. No telemetry, no services.
#define MyAppName "TalkBox Reference"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "TalkBox Reference"
#define MyAppExeName "TalkBox Reference.exe"

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
Source: "build\TalkBox Reference.vst3\*"; DestDir: "{autopf}\Common Files\VST3\TalkBox Reference.vst3"; Flags: ignoreversion recursubdirs createallsubdirs
Source: "build\TalkBox Reference.clap"; DestDir: "{autopf}\Common Files\CLAP"; Flags: ignoreversion
Source: "build\TalkBox Reference.exe"; DestDir: "{autopf}\TalkBox Reference"; Flags: ignoreversion
Source: "presets\factory_presets.json"; DestDir: "{autopf}\TalkBox Reference\presets"; Flags: ignoreversion

[Icons]
Name: "{group}\TalkBox Reference"; Filename: "{autopf}\TalkBox Reference\{#MyAppExeName}"

[UninstallRun]
; Clean uninstaller prompts Keep presets vs Remove all (see docs/07)
