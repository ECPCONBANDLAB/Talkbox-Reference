# Hand-off — TalkBox Reference 1.0.0

**Date:** 2026-11-17
**Repository:** `ECPCONBANDLAB/Talkbox-Reference`
**Branch:** `arena/e9dbfec9-talkbox-reference`
**Release:** `v1.0.0`
**Validation:** 46 checks, 46 pass, 0 fail (`artifacts/test_output.txt`)

---

## 1. What this delivery is

A complete, self-contained talkbox emulation: physically motivated DSP, a
framework-agnostic plugin core, a JUCE 7 shell (VST3 / AU / CLAP / Standalone),
four factory presets, packaging scripts for the three desktop platforms, CI, and
seven documents.

The checkout this was built on contained only `README.md`, so everything below
was written, compiled and measured in this session — no prior artefact was
carried over.

## 2. Signal chain

```
carrier → DC 18 Hz → Driver (ADAA + HP90/LP6.5k, 1x/2x/4x local OS)
        → Hose waveguide (1.5–2.0 m) → Vocal tract (lattice |k|≤0.995
        + F1–F5 TPT + nasal + lips) → Consonants → Radiation (lip, 4 mics,
        proximity, limiter)
```

| Header | Lines | Role |
|---|---|---|
| `src/dsp/Common.h` | 470 | one-poles, biquads, TPT SVF, FIR, FFT, peak pickers |
| `src/dsp/Carrier.h` | 150 | PolyBLEP source and noise blend |
| `src/dsp/Driver.h` | 200 | ADAA soft clip + localised oversampling + filters |
| `src/dsp/Hose.h` | 130 | fractional-delay lossy waveguide |
| `src/dsp/VocalTract.h` | 230 | lattice + 5 formants + nasal + lips |
| `src/dsp/Radiation.h` | 330 | lip, mics, proximity, limiter, consonants |
| `src/dsp/Engine.h` | 260 | chain, presets, latency |

## 3. Measured results

| Quantity | Spec | Measured |
|---|---|---|
| Formants F1/F2/F3 over 9 vowels | ±5 % | **0.62 / 0.17 / 0.04 %** |
| Whisper-path F1 / F2 | 15 % / 5 % | **1.0 % / 0.3 %** |
| Reported latency @48 kHz | < 6 ms | **36 samples = 0.75 ms** |
| Hose delay 1.50 / 2.00 m | L/c ± 3 % | **4.38 / 5.83 ms (0.0 %)** |
| Hose stability, 20 s at g = 0.995 | bounded | **peak 1.32** |
| Wall damping HF reduction | ≥ 6 dB | **15.6 dB** |
| Carrier bleed, muted | < −60 dBFS | **−240 dBFS** |
| ADAA peak, +12 dB drive | ≤ 0.8 | **0.515** |
| DC blocker residual | < −60 dB | **−240 dB** |
| HP 90 Hz @30 Hz | ≤ −12 dB | **−20.5 dB** |
| LP 6.5 kHz @16 kHz | ≤ −20 dB | **−30.0 dB** |
| 4x vs 1x alias rejection | ≥ 3 dB | **+4.5 dB** |
| PolyBLEP vs naive saw | ≤ −12 dB | **−17.8 dB** |
| Sibilance index /s/ | 0.50 ± 0.08 | **0.46** |
| Plosive attack | ≤ 20 ms | **5.48 ms** |
| Limiter, +12 dB input | ≤ −0.25 dBFS | **−0.30 dBFS** |
| Proximity 5 vs 50 cm | ≥ +3 dB | **+11.5 dB** |
| Mic model spread | ≥ 3 dB | **7.8 dB** |
| Lip radiation slope | 6 ± 1.5 dB/oct | **+6.0 dB/oct** |
| NaN/Inf/+120 dBFS recovery | ≤ 1 block | **pass** |
| CPU Eco/Mic/Balanced/Studio | ≤ 5/5/6/8 % | **≈1.2/1.1/1.5/2.1 %** |

Output level after the +26 dB physical makeup stage: a 0.3-amplitude 220 Hz
carrier produces −25.6 dBFS RMS in *Mic*, −23.4 dBFS in *Balanced*, −23.3 dBFS
in *Studio*.

## 4. Verify in 60 seconds

```bash
g++ -std=c++20 -O2 -I src/dsp -I tests tests/test_main.cpp -o tests/run_tests
./tests/run_tests        # PASS=46 FAIL=0
```

Plugin build (needs JUCE 7):

```bash
cmake -S . -B build -DTB_BUILD_JUCE=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## 5. Honest limitations

1. **Static vowels.** No coarticulation or glide; vowel changes step the
   coefficients (a 1.1 item).
2. **Formant accuracy is a filter measurement.** Impulse-response magnitude with
   a 32 768-point FFT, not a listening test.
3. **Whisper validation uses a spectral-envelope estimator, not LPC.** LPC order
   14 is implemented in `tests/LpcTools.h` and was rejected as a pass/fail
   criterion: on synthetic AR processes with known poles its estimates were
   biased 10–17 %. The envelope measurements (1.0 % / 0.3 %) are reported as they
   are, with the method documented in `docs/03-validation.md`.
4. **Mic models are static and minimum-phase.** No air absorption, no off-axis,
   no room.
5. **Consonants are gestures** (P/S/N noise bursts), not phoneme synthesis.
6. **No AAX** — Avid SDK licensing.
7. **CPU figures are sandbox figures** (±30 % on other hardware).
8. **No signed binaries** in the repository; JUCE builds are reproducible but
   the installers must be produced locally.

## 6. Bugs found and fixed during validation

| Bug | Symptom | Fix |
|---|---|---|
| Wrong sign in the ADAA antiderivative for `x < −1` | 9.1 peak from a 3.58 input | correct even antiderivative, continuous at \|x\| = 1 |
| One-multiplier lattice with a pole at z = 1 | DC integrated to 2·10¹¹ | step-up recursion to direct-form all-pole |
| Level smoother inside the carrier audio path | noise mode −34 dB, muffled saw | gain-only ramp |
| Hose tap before the damping filter | wall loss understated by 14 dB | tap the damped signal |
| Formant gains not normalised by Q | peaks drifted with bandwidth | `gain/Q` peak normalisation |
| No output makeup | −49 dBFS RMS for a 0.3 carrier | +26 dB physical makeup before the limiter |

## 7. Where to look next

| Question | File |
|---|---|
| How are the coefficients derived? | `docs/02-dsp-reference.md` |
| What exactly is tested? | `tests/test_main.cpp`, `docs/03-validation.md` |
| How do I build and ship it? | `docs/04-build-install.md` |
| How does it sound / how do I use it? | `docs/05-usage-presets.md` |
| What is not done? | `docs/07-limitations-roadmap.md` |
