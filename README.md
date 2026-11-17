# TalkBox Reference

A physically grounded, ultra-low-CPU emulation of the talkbox signal chain —
carrier, driver, hose waveguide, vocal tract, lip radiation and microphone — in
dependency-free C++20, with a JUCE 7 plugin shell (VST3 / AU / CLAP /
Standalone).

```
guitar ─► driver ─► hose 1.75 m ─► vocal tract ─► mic ─► amp
                                    /a/ /i/ /u/ …
```

**Status:** 1.0.0 · **Validation:** 46/46 checks pass · **CPU:** ~1–2 % of one
core at 48 kHz / 128 samples.

## Quick start (no dependencies, ~5 s)

```bash
g++ -std=c++20 -O2 -I src/dsp -I tests tests/test_main.cpp -o tests/run_tests
./tests/run_tests
# 46 checks, PASS=46 FAIL=0
```

## Build the plugin

```bash
cmake -S . -B build -DTB_BUILD_JUCE=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --config Release
```

Artefacts: `build/TalkBoxReference_artefacts/Release/{VST3,CLAP,AU,Standalone}`.
See [`docs/04-build-install.md`](docs/04-build-install.md) for platform notes and
installers.

## Signal chain

| Stage | Implementation | File |
|---|---|---|
| Carrier | External input or PolyBLEP saw/square/pulse/noise | `src/dsp/Carrier.h` |
| Driver | DC 18 Hz → ADAA cubic clip (1x/2x/4x local OS) → HP 90 Hz / LP 6.5 kHz | `src/dsp/Driver.h` |
| Hose | Lossy waveguide 1.5–2.0 m, 4-point Lagrange fractional delay | `src/dsp/Hose.h` |
| Vocal tract | Reflection lattice (\|k\| ≤ 0.995) + 5 TPT formants F1–F5 + nasal + lips | `src/dsp/VocalTract.h` |
| Radiation | Lip +6 dB/oct, 4 mic models, proximity, look-ahead limiter, consonants | `src/dsp/Radiation.h` |
| Engine | Chain, presets, latency reporting | `src/dsp/Engine.h` |

## Measured results

| Quantity | Spec | Measured |
|---|---|---|
| Formants F1 / F2 / F3, 9 vowels | ±5 % | **0.62 / 0.17 / 0.04 %** |
| Whisper-path F1 / F2 | 15 / 5 % | **1.0 / 0.3 %** |
| Latency at 48 kHz | < 6 ms | **0.75 ms** (36 samples) |
| Hose delay 1.50 / 2.00 m | L/c ± 3 % | **4.38 / 5.83 ms** |
| Bleed (carrier muted) | < −60 dBFS | **−240 dBFS** |
| Hose stability, 20 s @ g = 0.995 | bounded | **peak 1.32** |
| Limiter with +12 dB input | ≤ −0.25 dBFS | **−0.30 dBFS** |
| CPU Eco / Mic / Balanced / Studio | ≤ 5 / 5 / 6 / 8 % | **≈1.2 / 1.1 / 1.5 / 2.1 %** |

Full transcript, method notes and the exact test list:
[`docs/03-validation.md`](docs/03-validation.md) and
[`artifacts/test_output.txt`](artifacts/test_output.txt).

## Presets

| Preset | Use |
|---|---|
| **Eco Pad** | 1x oversampling, internal saw — cheapest |
| **Mic** | External carrier, minimal drive, 10 cm dynamic |
| **Balanced** | Default: 2x oversampling, 6 dB drive |
| **Studio** | 4x oversampling, condenser at 22 cm, 1.90 m hose |

## Documentation

| Doc | Contents |
|---|---|
| [01 Overview](docs/01-overview.md) | Architecture, design rules, latency budget |
| [02 DSP reference](docs/02-dsp-reference.md) | Equations, coefficients, vowel table |
| [03 Validation](docs/03-validation.md) | 46 checks, measured metrics, method notes |
| [04 Build & install](docs/04-build-install.md) | CMake/JUCE, installers, troubleshooting |
| [05 Usage](docs/05-usage-presets.md) | Controls, presets, recipes, automation |
| [06 Performance](docs/06-performance.md) | CPU profile, real-time safety, what was rejected |
| [07 Limitations](docs/07-limitations-roadmap.md) | Honest limitations, known issues, roadmap |

## Repository layout

```
src/dsp/      7 header-only DSP blocks + engine (no dependencies)
src/plugin/   framework-agnostic core + JUCE 7 shell
src/ui/       layout spec and SVG mock-up
presets/      4 factory presets (JSON)
tests/        46-check suite + LPC analysis helper
packaging/    Inno Setup, macOS/Linux installer, portable bundle
docs/         01–07
artifacts/    hand-off, test transcript, checksums, manifest
```

## Design rules

1. No allocation or locks on the audio thread.
2. No denormals or NaN propagation: hostile input is contained within one block.
3. Sample-rate independent (validated at 44.1–192 kHz, blocks 16–2048).
4. Small, reported latency: 36 samples at any rate, plus the hose acoustics
   reported separately.
5. Header-only DSP: the plugin shell is the only framework-aware code.

## License

MIT for the DSP core, plugin code and documentation (see [LICENSE](LICENSE)).
Builds that link JUCE are additionally subject to JUCE's own terms (AGPLv3 or a
commercial JUCE licence); this repository ships no JUCE code.

## Citing / credits

Vowel targets follow the classic Peterson–Barney / Klatt-style formant sets for
an adult male tract at 17 cm. Waveguide and lattice formulations follow the
standard digital-waveguide and Kelly–Lochbaum literature; TPT filters follow
Zavalishin's topology-preserving transform.
