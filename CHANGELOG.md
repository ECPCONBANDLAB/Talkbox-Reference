# Changelog

All notable changes to TalkBox Reference. Format: Keep a Changelog;
versioning: SemVer.

## [1.0.0] — 2026-11-17

First public release.

### Added

- **DSP core** (`src/dsp/`, header-only C++20, zero dependencies):
  - `Carrier` — external input or PolyBLEP saw / square / pulse / noise.
  - `Driver` — DC block 18 Hz, drive, first-order ADAA cubic soft clip,
    localised 1x/2x/4x oversampling, HP 90 Hz / LP 6.5 kHz.
  - `Hose` — lossy digital waveguide 0.5–3.0 m, 4-point Lagrange fractional
    delay, wall damping, contractive loop.
  - `VocalTract` — 6-section reflection lattice (|k| ≤ 0.995) + five TPT
    formant resonators (F1–F5) + nasal branch + lip aperture, 9 vowels.
  - `Radiation` — lip radiation (+6 dB/oct), four microphone models, proximity
    effect, look-ahead limiter, consonant bridge (P/S/N).
  - `Engine` — full chain, preset table, latency and hose-delay reporting.
- **Plugin** — framework-agnostic core (`tb::TalkboxApi`) plus a JUCE 7 shell
  producing VST3, AU, CLAP and Standalone targets.
- **UI** — 720 × 460 dark interface with three panel groups, preset selector,
  articulation buttons, peak meter and latency readout; layout spec and SVG
  mock-up in `src/ui/`.
- **Presets** — Eco Pad, Mic, Balanced, Studio (`presets/factory_presets.json`).
- **Validation** — 46-check suite covering formants, whisper path, driver
  filters, oversampling, hose timing/stability, consonants, radiation, limiter,
  hostile-input recovery and CPU budgets.
- **Packaging** — Inno Setup script, macOS/Linux installer with uninstall,
  portable bundle layout.
- **CI** — GitHub Actions: DSP tests on Linux/macOS/Windows, ASan/UBSan run,
  JUCE plugin build on Linux.
- **Docs** — 01 overview, 02 DSP reference, 03 validation, 04 build/install,
  05 usage, 06 performance, 07 limitations and roadmap.

### Fixed during development (kept here for the record)

- ADAA antiderivative had the wrong sign on the `x < -1` branch, producing a
  discontinuity and a 9.1 peak from a 3.58 input. Now bounded at 0.667.
- The all-pole lattice used a one-multiplier-per-section structure with a pole
  at z = 1, integrating DC to ~2·10¹¹. Replaced with the step-up recursion into
  direct form.
- Carrier level smoothing sat in the audio path, low-passing the carrier and
  attenuating noise mode by ~34 dB. Now a gain-only ramp.
- Hose output tap bypassed the damping filter, understating wall loss.
- Parallel formant gains were not normalised by Q, so peaks drifted with
  bandwidth. Now `gain/Q` yields an exact peak value of `gain`.

### Known limitations

- Static vowels, no coarticulation or glide (planned for 1.1).
- Consonants are triggered gestures, not phoneme synthesis.
- Microphone models are static, minimum-phase, with no room.
- No AAX (Avid SDK licensing).
