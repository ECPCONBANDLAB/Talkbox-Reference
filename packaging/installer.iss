; TalkBox Reference 1.0.0 — Inno Setup 6 script
; Build: iscc packaging\installer.iss /DVersion=1.0.0
#define MyAppName "TalkBox Reference"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "ECP Conband Lab"
#define MyAppURL "https://github.com/ECPCONBANDLAB/Talkbox-Reference"
#define Vst3Dir "{commoncf}\VST3\TalkBox Reference.vst3"

[Setup]
AppId={{7C1F5E62-3D8B-4A21-9C0E-2B7A5D9E4F10}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
UninstallDisplayIcon={app}\TalkBox Reference.exe
OutputBaseFilename=TalkBoxReference-{#MyAppVersion}-win64
Compression=lzma2/max
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
WizardStyle=modern
DisableProgramGroupPage=yes
LicenseFile=..\LICENSE
PrivilegesRequired=admin
SetupLogging=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Types]
Name: "full"; Description: "Standalone + VST3 + CLAP"
Name: "pluginonly"; Description: "VST3 + CLAP only"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "standalone"; Description: "Standalone application"; Types: full custom
Name: "vst3"; Description: "VST3 plugin"; Types: full pluginonly custom
Name: "clap"; Description: "CLAP plugin"; Types: full pluginonly custom
Name: "presets"; Description: "Factory presets"; Types: full pluginonly custom

[Files]
Source: "..\build\TalkBoxReference_artefacts\Release\Standalone\TalkBox Reference.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\build\TalkBoxReference_artefacts\Release\VST3\TalkBox Reference.vst3\*"; DestDir: "{commoncf}\VST3\TalkBox Reference.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\TalkBoxReference_artefacts\Release\CLAP\TalkBox Reference.clap"; DestDir: "{commoncf}\CLAP"; Components: clap; Flags: ignoreversion
Source: "..\presets\factory_presets.json"; DestDir: "{app}\presets"; Components: presets; Flags: ignoreversion
Source: "..\docs\*"; DestDir: "{app}\docs"; Flags: ignoreversion recursesubdirs
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\CHANGELOG.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\TalkBox Reference.exe"; Components: standalone
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\TalkBox Reference.exe"; Tasks: desktopicon; Components: standalone

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"

[Run]
Filename: "{app}\TalkBox Reference.exe"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\presets"
Type: dirifempty; Name: "{app}"
