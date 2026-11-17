# 02 — DSP reference

All equations are given for a sample rate `fs`. The implementation is in
`src/dsp/`, header-only and namespace `tb`.

## 1. Carrier — `Carrier.h`

Modes: `External`, `Saw`, `Square`, `Pulse`, `Noise`. The internal oscillator is
band-limited with PolyBLEP:

```
saw[n]    = 2φ - 1 - blep(φ, Δ)
square[n] = sgn(φ < 0.5) + blep(φ, Δ) - blep(φ + 0.5, Δ)
pulse[n]  = sgn(φ < w) + blep(φ, Δ) - blep(φ + 1 - w, Δ) - (2w - 1)
```

where `Δ = f/fs` and `blep` is the usual two-piece polynomial residual. Phase is
wrapped in `[0,1)`. The level smoother is a **gain** ramp (5 ms), never a filter
on the audio path — an early revision low-passed the carrier itself, which
attenuated noise mode to −40 dB; the suite caught it (test B3).

## 2. Driver — `Driver.h`

```
x → DC block 18 Hz → × g_drive → ADAA soft clip (1x/2x/4x) → HP 90 Hz → LP 6.5 kHz → trim
```

Soft clip and its antiderivative:

```
f(x) = x - x³/3                 |x| ≤ 1          (odd)
     = sgn(x)·2/3               |x| > 1
F(x) = x²/2 - x⁴/12             |x| ≤ 1          (even, F(0)=0)
     =  2x/3 - 1/4              x > 1
     = -2x/3 - 1/4              x < -1
```

First-order ADAA:

```
y[n] = (F(x[n]) - F(x[n-1])) / (x[n] - x[n-1])      |Δx| > 1e-6
     = f((x[n] + x[n-1]) / 2)                        otherwise
```

`F` must be continuous at `|x| = 1`; the first implementation had the sign of
the left branch flipped, which produced a discontinuity and a 9.1 peak from a
3.58 input. Correcting the branch dropped the peak to 0.667 as theory requires
(test C1).

Localised oversampling: zero-stuffing → Blackman-windowed sinc FIR (4·factor+1
taps, cutoff 0.46/ factor) → ADAA → FIR → decimate. Both FIRs contribute
`(taps-1)/2` at the oversampled rate, i.e. **4 base-rate samples** of latency for
factor 2 or 4. Measured alias improvement at 9 kHz with +18 dB drive: **4.5 dB**
versus 1x (test C5); ADAA already removes most of the folding by itself.

## 3. Hose — `Hose.h`

A lossy digital waveguide of length `L` (0.5–3.0 m, default 1.75 m):

```
D  = L · fs / c            (c = 343 m/s)
y  = Lagrange₄(x[n-D] , μ)  (4-point, 3rd order)
h  = onepole(y, f_damp)
w  = x + g_fb · 10^(-loss·L/20) · h
```

The mouth-end tap returns the **damped** signal, because the air column itself
is lossy; returning the raw delay understated damping by 14 dB in test D4 (1.0 dB vs 15.6 dB of measured HF reduction).
Measured delay: 4.38 ms at 1.50 m, 5.83 ms at 2.00 m (0.0 % error). The loop is
contractive: `|g| ≤ 0.9995` is enforced after the wall-loss term, and 20 s at
maximum feedback peaks at 1.5 (test D3).

## 4. Vocal tract — `VocalTract.h`

Two cooperating blocks:

**a) Reflection lattice.** Six sections with normalised reflection coefficients
`|k_m| ≤ 0.995` (default `k_m = (0.42 - 0.055m)·amount`). The coefficients are
converted to a direct-form all-pole polynomial with the step-up recursion

```
a_i^(m) = a_i^(m-1) + k_m · a_(m-i)^(m-1),   a_m^(m) = k_m
```

and realised as `y[n] = x[n] - Σ a_i y[n-i]`. This is stable for every `|k|<1`,
unlike the naive "one multiplier per section" lattice, whose pole sits at z = 1
and integrates DC into a 2·10¹¹ blow-up (also caught by the suite).

**b) Formant bank.** Five TPT (topology-preserving transform) state-variable
band-passes in parallel:

```
g = tan(π fc/fs),  k = 1/Q,  Q = fc/BW
a1 = 1/(1+g(g+k)), a2 = g·a1, a3 = g·a2
v3 = x - s2;  v1 = a1 s1 + a2 v3;  v2 = s2 + a2 s1 + a3 v3
s1 = 2v1 - s1;  s2 = 2v2 - s2;  out = v1 · (gain/Q)
```

`v1` peaks at `Q`, so the `gain/Q` factor makes the peak exactly `gain`, which
keeps the parallel sum's local maxima on the target frequencies.

Vowel table (Hz / bandwidth, male reference, VTL = 1.0):

| vowel | F1 | F2 | F3 | F4 | F5 |
|---|---|---|---|---|---|
| i | 270/60 | 2290/90 | 3010/120 | 3500/140 | 4500/180 |
| I | 390/70 | 1990/100 | 2550/130 | 3300/150 | 4400/180 |
| e | 530/70 | 1840/100 | 2480/130 | 3300/150 | 4400/180 |
| E | 660/80 | 1720/110 | 2410/130 | 3300/150 | 4400/180 |
| ae | 730/90 | 1090/110 | 2440/140 | 3350/160 | 4400/180 |
| a | 700/80 | 1220/100 | 2600/130 | 3400/150 | 4500/180 |
| open o | 570/70 | 840/90 | 2410/130 | 3300/150 | 4400/180 |
| o | 430/60 | 1020/90 | 2340/130 | 3300/150 | 4400/180 |
| u | 300/60 | 870/80 | 2240/120 | 3300/150 | 4400/180 |

`VTL` divides every formant frequency; `formantShift` multiplies by
`2^(semitones/12)`. The nasal branch adds a 280 Hz pole with a 900 Hz
anti-resonance scaled by `nasal`. The lip stage applies `0.12 + 0.88·aperture`
and a low-pass whose corner runs from 0.9 to 11.9 kHz as the lips open.

## 5. Radiation and microphone — `Radiation.h`

```
lip:        y = 8·(x - x[n-1]) → onepole 9 kHz     (+6 dB/oct, measured 6.0)
mic:        HP → proximity low shelf → presence → high shelf
proximity:  +6 dB · clamp(0.15/d, 0, 4) · modelScale  at 130 Hz
```

| Model | HP | Presence | Shelf |
|---|---|---|---|
| Dynamic | 55 Hz | +3 dB @ 5.5 kHz | −4 dB @ 11 kHz |
| Small diaphragm | 30 Hz | +1.5 dB @ 8 kHz | +1 dB @ 12 kHz |
| Condenser | 35 Hz | +2 dB @ 10 kHz | +2.5 dB low @ 220 Hz |
| Ribbon | 45 Hz | +1.5 dB @ 3 kHz | −7 dB @ 7 kHz |

A **+26 dB make-up stage** sits after the mic colouration and before the
limiter. It is not a mixing decision: a 220 Hz carrier is ~15 dB below F1 and the
lip differentiator removes another 13 dB there, so the physical chain is
inherently quiet. Without the make-up stage a 0.3-amplitude carrier produced
−49 dBFS RMS; with it, the same carrier produces −23 dBFS in *Balanced*.

Limiter: feed-forward, 32-sample look-ahead, instantaneous attack, 80 ms
release, then a hard clamp at the ceiling (−0.3 dBFS by default).

**Consonant bridge.** `S`, `Sh`, `Plosive` and `Nasal` envelopes generate
filtered noise bursts that are summed into the tract output. The `S` branch
weight is `0.55·e(t)·(1.05·HP(3.8k)→peak(6.5k) + 0.42·LP(2.6k))`, which gives a
sibilance index of 0.46 (fraction of energy in 4–10 kHz) — see test G1.

## 6. Engine — `Engine.h`

Per sample, in order: carrier → driver → hose → tract → consonant sum →
radiation → output gain. Stage bypass flags (`driverEnabled`, `hoseEnabled`,
`radiationEnabled`) exist for measurements and for a future "hose only" mode.
`latencySamples()` returns driver oversampler + limiter look-ahead only.

## 7. Numerical hygiene

* Every recursive state is checked with `isfinite`; a non-finite value resets the
  state instead of propagating it.
* The ADAA branch uses `|Δx| > 1e-6` to avoid division by noise.
* The hose clamps its loop gain to 0.9995 and the limiter clamps its output to the
  ceiling, so +120 dBFS input produces a bounded, limited signal (test I2).
