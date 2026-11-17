# TalkBox Reference

**Physically accurate hardware talkbox emulation — mic-playable, ultra-low CPU.**

A production-grade real-time audio plugin + standalone that faithfully emulates the complete electro-acoustic-articulatory chain of a physical hardware talkbox: carrier → compression driver → vinyl hose → time-varying vocal tract → lip radiation → close-talk mic. Playable via **live microphone (spoken or whispered)**, **2D Vowel Pad**, or **MIDI/automation**, with a built-in talkbox-tuned carrier synth for instant jamming.

> Designed and validated as a physically grounded, ultra-low-CPU source-filter talkbox emulation.

## Highlights

- **True source-filter physics** (Fant 1960; Kelly & Lochbaum 1962; Markel & Gray 1976) — not a wah, not a vocoder filterbank.
- **Mic-playable:** 16 kHz-decimated LPC vocal-tract tracker (whisper + speech), consonant/sibilance bridge, zero pitch bleed (Voice Bleed 0%).
- **Ultra-low CPU:** Eco 0.32% Pad / 0.61% Mic, Balanced 0.42%/0.71%, Studio 0.53%/0.82% (48 kHz/128, measured — see `docs/05`).
- **Unconditionally stable:** normalized lattice (LAR, |k|≤0.995) + TPT ZDF, ADAA driver, FTZ/DAZ, NaN-safe. 40/40 headless tests pass.
- **100% offline, private, trivially uninstallable.** No telemetry, no services, no accounts. Portable mode supported.

## Quick start

1. Load on an instrument track. Set **Carrier = Internal Talkbox Synth** to play immediately (MIDI/keyboard).
2. Drag the **Vowel Pad** to morph vowels, or set **Source = Live Mic Control** and talk/whisper into your mic.
3. Leave **Quality = Balanced** (default). See `docs/06_User_Manual.md` for DAW sidechain setup.

## Build

```bash
# Headless DSP tests (no JUCE needed)
g++ -std=c++20 -O2 -I src/dsp tests/test_main.cpp -o tests/run_tests && ./tests/run_tests
# expect: Summary: PASS=40 FAIL=0

# Full plugin (requires JUCE 7+)
cmake -B build -DTB_BUILD_JUCE=ON -DJUCE_PATH=/path/to/JUCE
cmake --build build --config Release
```

Formats: 64-bit **VST3, AU (macOS), Standalone**. Windows / macOS (Universal) / Linux. No CLAP build (JUCE 7.0.12 has no CLAP wrapper) and no AAX (Avid SDK licensing) — see `docs/07`.

## No installers in v1.0.0

**There is no `Setup.exe`, `.dmg`, `.pkg` or portable ZIP.** The release ships source, docs and presets. That is deliberate: `src/plugin/` links JUCE 7.0.12, which is GPLv3-or-commercial, so publishing binaries would require GPLv3 terms (this repo is MIT) — and there is no code signing. Build from source with the two commands above; details in `docs/07` §0–1. See `LICENSE-NOTES.md`.

## Repository

```
src/dsp/       dependency-free C++20 engine (headless-testable)
src/plugin/    JUCE AudioProcessor (VST3/AU/Standalone glue)
src/ui/        vector GUI components (Vowel Pad, knobs, visualizer)
presets/       factory presets (Mic-Ready Funk Lead, Whisper Talkbox, ...)
tests/         headless test + CPU benchmark suite (ctest / Catch2-ready)
packaging/     installers, portable mode, clean uninstallers
docs/          7 formal documents (01–07)
artifacts/     test report, checksums, release manifest, handoff
.github/workflows/ci.yml
CMakeLists.txt, README.md, LICENSE, CHANGELOG.md
```

## Docs

1. `docs/01_Research_and_Physics_Report.md` — verified literature & acoustic baseline
2. `docs/02_DSP_and_Accuracy_Specification.md` — LPC/LAR/ADAA/consonant-bridge math
3. `docs/03_GUI_and_UX_Specification.md` — Simple/Advanced vector GUI
4. `docs/04_Performance_and_CPU_Specification.md` — budgets & measured CPU
5. `docs/05_Test_and_Validation_Report.md` — formant/mic/stability metrics
6. `docs/06_User_Manual.md` — mic quick-start, whisper tips, feedback guide
7. `docs/07_Install_and_Uninstall_Guide.md` — OS packaging & clean uninstall

Final handoff: `artifacts/Final_Handoff_Report.md`

## License

MIT — see `LICENSE`. Plain-language summary in [`LICENSE-NOTES.md`](LICENSE-NOTES.md).

- `src/dsp/` has no dependencies: MIT, unconditionally.
- `src/plugin/` builds on JUCE 7.0.12, which is dual-licensed **GPLv3 or a paid
  commercial licence**. Publishing *source* requires no JUCE licence;
  distributing *binaries* requires GPLv3 or a commercial JUCE licence.
- VST3 is MIT-licensed, so the format itself imposes no terms here.

## Trademark note

"TalkBox" is a historical hardware name. This is a descriptive research reference; no hardware, logos, or trademarks ship with it. Fallback neutral name if needed: **Vocal Hose Reference** (rename via `PLUGIN_NAME`).
