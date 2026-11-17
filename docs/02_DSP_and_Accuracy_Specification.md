# TalkBox Reference — 02 DSP & Accuracy Specification

**Engine:** `src/dsp/` (dependency-free C++20, headless-testable, zero alloc on audio thread)  
**Chain:** Carrier → DC → Driver (ADAA+BP) → Hose → Vocal Tract (Lattice+TPT) → Lip/Mic → Consonant Bridge → Mix/Limiter

---

## Stage A — Carrier input, conditioning & internal synth

- **External carrier:** input gain calibration, DC blocker (1st-order HP ~18 Hz, `TbCore.h:DcBlocker`), transient-preserving (no lookahead, no oversampling of full chain).
- **Internal Talkbox Synth** (`TbSynthDriver.h:TalkboxSynth`): PolyBLEP saw/pulse, 4-voice poly-lite, glide/portamento (exp smoothing), vibrato (LFO in semitones), pitch-bend, −1-octave sine sub. Modeled after bright Minimoog/DX-style harmonic carriers. Selectable when no external carrier is fed or in Standalone with mic + MIDI/computer keyboard. Alias suppression via PolyBLEP (no oversampling cost).

## Stage B — Compression driver & horn throat (ADAA)

- **Passband:** HP ~90 Hz + LP ~6.5 kHz (TPT one-poles, `DriverBandpass`). Horn high-cut adjustable 5.5–7.5 kHz.
- **Nonlinearity:** cubic soft-clip with asymmetry:
  - `f(x) = x + a·x² − x³/3` for |x|<1, `sign(x)·(2/3+a)` outside (a = diaphragm asymmetry, −0.5…0.5).
  - Antiderivative `F₁(x) = x²/2 + a·x³/3 − x⁴/12` inside; linear extension outside (continuous at ±1).
  - First-order ADAA: `y[n] = (F₁(x[n])−F₁(x[n−1]))/(x[n]−x[n−1])`, fallback `f((x[n]+x[n−1])/2)` when |Δx|<1e-6. Zero-latency (Eco/Live).
  - Dynamic excursion compression: envelope follower reduces drive on loud signals (`comp` 0…1).
- **Oversampling (localized, driver only):** Eco 1× (ADAA only), Balanced 2× linear-interp, Studio/Creative 4×. Production polyphase IIR can replace linear interp without API change; test suite validates linear version meets aliasing targets for talkbox band (90 Hz–6.5 kHz).

## Stage C — Vinyl hose waveguide

- **Physical:** 1.5–2.0 m vinyl tube, c=343 m/s → 4.5–6 ms propagation. Live/Eco keeps propagation latency ~0 (comb only); Studio/Creative enables physical delay (reported PDC).
- **Implementation** (`TbVocalTract.h:HoseWaveguide`): circular delay buffer (4096 max, covers 192 kHz), 1-pole loop damping (viscous/thermal HF loss), mouth-closure reflection scalar (closing lips loads/damps hose). Preallocated, no alloc.

## Stage D — Time-varying articulatory vocal tract (core)

**Architecture:** Hybrid Normalized Lattice (LAR domain) + TPT resonator cascade (per spec allowance). Audio-rate filter is TPT (stable under fast modulation); control is lattice/LAR (guaranteed |k|<1).

- **Pad/Articulatory mode:** `padToFormants(X,Y,VTL,rounding)` via inverse-distance weighting (IDW, w∝1/d³) over 9 vowel anchors (exact at anchors, smooth between). Articulatory controls map: Tongue X→X, Jaw/Tongue Height→Y, Lip Rounding lowers F2/F3 up to 15%, Mouth Opening controls lip-closure damping, Nasal controls anti-resonance depth, VTL scales all formants by 1/VTL, Glide/Inertia controls ballistic smoothing.
- **Audio filter:** cascade of 5 TPT lowpass resonators (F1–F5, Q=F/B) + nasal notch (bandpass subtraction at ~250 Hz) + lip-closure lowpass (800 Hz–6.8 kHz by opening). TPT SVF (Zölzer): `g=tan(π·fc/Fs)`, `k=1/Q`, `a1=1/(1+g(g+k))`, `a2=g·a1`, `a3=g·a2`; `v3=x−ic2`, `v1=a1·ic1+a2·v3`, `v2=ic2+a2·ic1+a3·v3`, `lp=v2`, `bp=v1`, `hp=x−k·v1−v2`, `ic1=2v1−ic1`, `ic2=2v2−ic2`. DC gain 1, peak gain Q, stable for Q>0.25.
- **Control rate:** formant targets updated every 16 (Balanced/Studio) or 32 (Eco) samples; TPT coefficients recomputed at control rate with ballistic smoothing (`a=1−glide·0.85`, clamped 0.05–0.95). No per-sample trig in inner loop (SIMD-friendly).
- **Stability:** |kᵢ|≤0.995, LAR smoothing, TPT ZDF, FTZ/DAZ, NaN/Inf sanitization per sample (`sanitize()`).

## Stage E — Microphone articulation engine (whisper + speech LPC + consonant bridge)

**Routing:** DAW Sidechain (bus 1), Main Input Split (L=carrier, R=mic), Standalone direct mic. All feed same `LpcAnalyzer`.

**Zero-alloc LPC tracker** (`TbLpcMic.h:LpcAnalyzer`):
- Decimate to 16 kHz: anti-alias 1-pole LP @7 kHz at input rate → fractional decimation (accumulator, handles 44.1–192 kHz) → shelf pre-emphasis at 16 kHz: `y = x + (x − LP800(x))` (flat below 800 Hz, +6 dB above; preserves F1 unlike differencing pre-emphasis which suppresses F1 by ~16 dB).
- Frame: 25 ms Hann window (400 @16k), 4 ms hop (64 @16k). Autocorrelation lags 0…P (P=16 for low-formant resolution: 1 ms lag coverage at 16 kHz; P=12 merges F1/F2 for /u/ — validated).
- Levinson-Durbin (O(P²), preallocated buffers) → reflection coefficients kᵢ, clamped |kᵢ|≤0.995.
- LAR smoothing: `LARᵢ=ln((1+kᵢ)/(1−kᵢ))`, ballistic smoothing, back via `k=tanh(LAR/2)`. Pole-damping radial factor implicit via clamp + bandwidth floor (30–500 Hz).
- **Whisper compatibility:** autocorrelation models spectral envelope, no pitch detection. Noise-excited whispered vowels (/i/, /ɑ/, /u/) track as accurately as voiced (validated: F1 errors 1–15%, F2 0–5%).
- **Pole extraction:** double-precision Durand-Kerner (80 iters, safeguarded, radius clamp) on A(z) reconstructed from kᵢ; spectral-peak fallback on |1/A(e^jw)| (200 bins) if divergence. VTL warping in mic mode: scale by 1/VTL (Bark/all-pass warping equivalent for talkbox band).
- **Hop throttling:** LPC analyze at 4 ms hop (not every control tick) — 12× CPU saving vs per-tick analysis. Between hops, hold last formants.

**Consonant & sibilance bridge:**
- Detector: ZCR + HF/LF ratio (HF via first-difference, LF via 1 kHz one-pole, frame-local). Noise ratio ~3.0, voiced ~0.06. Sibilance = clamp((ratio−0.8)/1.5)·clamp((ZCR−0.12)/0.25).
- When unvoiced > gate, blend high-passed mic transients (HP ~3 kHz via one-pole difference, de-ess scaled) into lip/mic stage with `Consonant Crispness` gain. Phase-coherent (same sample clock, no lookahead). Voice Bleed default 0% (no raw mic leak).

**Gate & hold:** RMS gate (calibratable threshold, default 0.008) with envelope smoothing. Behaviors: *Close Mouth Naturally* (authentic: mouth closes, tract damps), *Hold Last Vowel*, *Return to Pad Position*.

## Stage F — Lip radiation & close-talk mic

- **Lip radiation:** `y = x + (x − LP(x))·gain` (+6 dB/oct tilt), cutoff 400 Hz + opening·2000 Hz, gain 0.4+opening·0.6. Scales with Mouth Opening Area.
- **Mic models:** Classic Dynamic 57/58 (presence 3.8 kHz, Q 1.2), Crisp Studio Condenser (4.2 kHz), Warm Ribbon (2.8 kHz + dark shelf), Flat Direct (no presence). Proximity boost (180 Hz LP add, stronger at 1–5 cm), preamp saturation (tanh), ceiling limiter (−1 dBFS soft clip). Ambience 0% default (ultra-light early reflections available in Studio).

## Performance architecture

- **Decoupled rates:** audio-rate inner loops (per-sample TPT/ADAA) vs control-rate (16/32 samples) vs LPC hop (4 ms). Heavy trig/articulatory/LPC at control rate only.
- **Zero alloc/locks:** all buffers preallocated in `prepare()` (mic ring 1024, hose 4096, LPC scratch). No `new`/`vector` resize/`string` in `processBlock`. Lock-free atomics for visualizer taps.
- **FTZ/DAZ + sanitization:** `ScopedNoDenormals`, per-sample `sanitize()` (NaN/Inf→0, denormal flush, blowup clamp).
- **SIMD/fast math:** portable scalar (SSE2/AVX2 runtime dispatch in production, NEON on ARM64, scalar fallback); fast Padé/LUT for tan/tanh in control-rate paths. No AVX-512 requirement.

## Accuracy summary (measured, see 05)

- Engine formants (VTL=1.0, small-signal IR): F1 ≤2.3% error, F2 ≤1.4%, F3 ≤1.6% across 9 vowels (spec ±5% ✓).
- Pure tract: F1/F2 ≤5% ✓. VTL scaling exact (1/1.2=0.833).
- LPC whisper tracking: F1 1–15%, F2 0–5% (spec <6 ms latency, <−60 dB bleed ✓).
- CPU (48 kHz/128): Eco Pad 0.34%, Eco Mic 0.66%, Balanced Pad 0.45%, Balanced Mic 0.77%, Studio Pad 0.58%, Studio Mic 0.88% (all budgets ✓).
