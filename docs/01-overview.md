# 01 — Overview and architecture

**TalkBox Reference 1.0.0** is a physically grounded emulation of the talkbox
signal chain: a carrier (guitar or synth) drives a small loudspeaker inside a
hose, the hose feeds a vocal tract whose shape is set by a vowel, and the result
is picked up by a microphone. Every stage is a small, deterministic, dependency
free C++20 block.

## Why a "reference" implementation

Commercial talkbox emulations tend to be either convolution snapshots or ad-hoc
formant EQ. This project is built the other way around: start from the physics
(the acoustic tube, the driver, the lip radiation, the microphone) and keep the
DSP cheap enough to run at well under 2 % of one core. The result is a sound
that responds to the carrier's pitch and drive the way the real thing does, and
a code base small enough to audit in an afternoon.

## Signal chain

```
carrier in ──► Carrier ──► DC block 18 Hz ──► Driver ──► Hose waveguide ──┐
(external / PolyBLEP saw, square, pulse, noise)   ADAA + HP 90/LP 6.5 k   │
                                                   1x / 2x / 4x local OS  │
                                                                          ▼
   out ◄── Radiation + mic ◄── Consonant bridge ◄── Vocal tract ◄─────────┘
            lip radiation       P / S / N          lattice |k|≤0.995
            4 mic models                           + F1–F5 TPT bank
            proximity                              + nasal branch
            look-ahead limiter                     + lip aperture
```

| Stage | File | Responsibility |
|---|---|---|
| Carrier | `src/dsp/Carrier.h` | External input or PolyBLEP oscillator, noise blend, click-free gain ramp |
| Driver | `src/dsp/Driver.h` | DC block, drive, ADAA cubic soft clip, localised oversampling, HP 90 Hz / LP 6.5 kHz |
| Hose | `src/dsp/Hose.h` | Digital waveguide, 0.5–3.0 m, 4-point Lagrange fractional delay, wall damping |
| Vocal tract | `src/dsp/VocalTract.h` | Reflection lattice → 5 TPT formant resonators → nasal → lip aperture |
| Radiation | `src/dsp/Radiation.h` | Lip radiation, 4 mic models, proximity, limiter, consonant bridge |
| Engine | `src/dsp/Engine.h` | Sample-accurate chain, preset table, latency reporting |
| Plugin core | `src/plugin/TalkboxApi.h/.cpp` | Parameters, presets, state, block processing (no JUCE dependency) |
| JUCE shell | `src/plugin/PluginProcessor.*`, `PluginEditor.*` | VST3 / AU / CLAP / Standalone |

## Design rules

1. **No allocation and no locks on the audio thread.** Every buffer is sized in
   `prepare()`; `process()` only touches pre-allocated state.
2. **No denormal storms.** Every recursive state has a finite guard; hostile
   input (NaN, ±Inf, +120 dBFS, DC) is contained and recovers within one block.
3. **Sample-rate independence.** All coefficients are derived from the runtime
   sample rate; the suite exercises 44.1, 48, 96 and 192 kHz with block sizes
   from 16 to 2048.
4. **Latency is small and *known*.** Algorithmic latency is 36 samples at any
   rate (oversampler group delay + 32-sample limiter look-ahead) and is reported
   to the host through `setLatencySamples()`.
5. **Header-only DSP.** The whole engine is seven headers; the plugin shell is
   the only place that knows about a framework.

## Latency budget (48 kHz)

| Contribution | Samples | ms |
|---|---|---|
| Driver oversampler (2x/4x FIR pair) | 4 | 0.08 |
| Limiter look-ahead | 32 | 0.67 |
| **Reported total** | **36** | **0.75** |
| Hose acoustic delay (1.75 m, not PDC) | 245 | 5.10 |

The hose delay is part of the instrument, not of the processing: it is reported
separately in the UI and in `TalkboxEngine::hoseDelayMs()`.

## Repository map

```
src/dsp/       7 header-only DSP blocks + engine
src/plugin/    framework-agnostic core + JUCE 7 shell
src/ui/        layout specification and SVG mock-up
presets/       4 factory presets (JSON, mirrored in Engine.h)
tests/         46-check validation suite + LPC analysis helper
packaging/     Inno Setup script, macOS/Linux installer, portable bundle
docs/          01–07
artifacts/     hand-off report, test transcript, checksums, manifest
```
