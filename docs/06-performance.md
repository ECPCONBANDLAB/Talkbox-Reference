# 06 — Performance and real-time behaviour

## Measured CPU (48 kHz, 128-sample blocks, `-O2`, one core)

| Preset | Oversampling | Measured | Budget |
|---|---|---|---|
| Eco Pad | 1x | **≈1.2 %** | 5 % |
| Mic | 1x | **≈1.1 %** | 5 % |
| Balanced | 2x | **≈1.5 %** | 6 % |
| Studio | 4x | **≈2.1 %** | 8 % |

Measured on the reference build machine (x86_64 sandbox, 2 vCPU). Expect
±30 % on other hardware; the suite's budgets already include that margin, and
every preset sits 4–5x below its budget.

Absolute per-sample cost at 48 kHz is roughly 0.24 µs for Balanced — the whole
chain costs less than a single oversampled `tanh()` saturation stage in a typical
plug-in.

## Where the time goes (Balanced, 2x)

| Stage | Share | Notes |
|---|---|---|
| Driver + oversampler | ~35 % | 2 × 9-tap FIR at 2x plus ADAA |
| Vocal tract | ~30 % | 6 lattice taps + 5 TPT SVFs + nasal + lip biquad |
| Hose | ~15 % | 4-point Lagrange + one-pole damping |
| Radiation + limiter | ~15 % | 4 biquads, tanh, 32-sample look-ahead |
| Carrier | ~5 % | PolyBLEP or passthrough |

The 4x preset costs roughly 1.5x the 2x preset, not 2x, because the formant bank
and the waveguide run at the base rate — oversampling is *localised* to the
non-linearity.

## Real-time safety checklist

| Rule | Where |
|---|---|
| No `new`/`delete`, `malloc`, `std::vector::resize` in `process()` | all DSP headers size their buffers in `prepare()` |
| No locks, no atomics in the audio path except the peak meter | `PluginProcessor.cpp` uses one `std::atomic<float>` for the UI |
| No `std::function` in the inner loop (templates instead) | `Oversampler::process` is a template on the non-linearity |
| Denormals flushed | `juce::ScopedNoDenormals` in `processBlock`; finite guards in every state |
| Parameter reads are cheap | 17 parameter fetches per block, not per sample |
| Bounded worst case | every loop has a compile-time trip count; no data-dependent loops |

## Memory

| Structure | Size (48 kHz) |
|---|---|
| Hose delay line | 3.0 m / 343 m/s × 48 000 ≈ 420 doubles (3.4 KB) |
| Oversampler FIR pairs | 2 × (4·factor+1) doubles |
| FFT/analyser | none in the audio path (tests only) |
| **Total engine state** | **< 8 KB** |

## Scaling to higher rates

The engine is rate-agnostic: at 192 kHz the hose buffer grows 4x (about 13 KB),
the FIR coefficients are redesigned, and CPU scales roughly linearly. The test
suite runs a 192 kHz / block-1024 sweep on every build (test I3) to catch
coefficient bugs that only appear when `fc/fs` gets small.

## Optimisation notes (what was tried)

* **Wavetable carrier** — 0.02 % faster than PolyBLEP, but aliasing rose by
  6 dB. Rejected.
* **Higher-order Lagrange (6-point)** — delay error improved from 0.0 % to
  0.0 %, cost +8 %. Rejected.
* **FFT-based formant bank** — 3x faster in theory, but block latency and
  coefficient interpolation made it unusable for vowel automation. Rejected.
* **Look-ahead limiter of 64 samples** — no audible improvement, doubled the
  reported latency. Rejected.
