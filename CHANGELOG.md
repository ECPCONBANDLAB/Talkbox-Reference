# Changelog — TalkBox Reference

## [1.0.0] — 2026-11-17

### Added
- Complete dependency-free DSP engine (`src/dsp/`): PolyBLEP carrier synth, ADAA compression driver, hose waveguide, normalized LAR lattice + TPT vocal tract (F1–F5, nasal, lip-closure), 16 kHz LPC mic tracker (whisper + speech), consonant/sibilance bridge, lip radiation + close-mic stage.
- JUCE plugin shell (VST3/AU/CLAP/Standalone) with Simple/Advanced vector GUI, 2D Vowel Pad, mic quick-setup, preset browser, A/B, undo/redo, MIDI learn.
- 4 factory presets: Mic-Ready Funk Lead, Whisper Talkbox, Classic Guitar Hose, Pad Vowel Morphs.
- Headless test & benchmark suite: 40/40 PASS (formants ≤2.3%, LPC F1 1–15%, CPU all budgets, NaN-safe 44.1–192 kHz).
- OS packaging: Windows Inno Setup + portable ZIP, macOS .pkg/.dmg + uninstall command, Linux .tar.gz + install/uninstall scripts. PortableData mode.
- 7 formal docs (01–07) + final handoff report with verified citations.

### Notes
- AAX skipped (Avid SDK licensing). JUCE optional (`-DTB_BUILD_JUCE=ON`); DSP tests build without JUCE.
- Trademark: descriptive research name; fallback neutral name Vocal Hose Reference available.
