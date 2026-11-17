TalkBox Reference 1.0.0 — portable bundle
=========================================

Windows (x64)
  Run "TalkBox Reference.exe" from this folder. No installer, no registry keys.
  To use the plugin in a DAW, copy "TalkBox Reference.vst3" to %COMMONPROGRAMFILES%\VST3\.

macOS (universal)
  Drag "TalkBox Reference.app" anywhere. For the plugins, copy:
    TalkBox Reference.vst3 -> ~/Library/Audio/Plug-Ins/VST3/
    TalkBox Reference.component -> ~/Library/Audio/Plug-Ins/Components/
    TalkBox Reference.clap -> ~/Library/Audio/Plug-Ins/CLAP/

Linux (x86_64)
  Run "./TalkBox Reference" (Standalone). Plugins go to ~/.vst3 and ~/.clap.

Factory presets live in presets/factory_presets.json; the Standalone also
restores its last session from the standard user settings folder.

Verify integrity with SHA256SUMS.txt (artifacts/SHA256SUMS in the repository).
