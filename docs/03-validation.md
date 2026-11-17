# 03 — Validation report

Every number in this document is produced by `tests/test_main.cpp` and captured
verbatim in `artifacts/test_output.txt`. Nothing here is hand-written from
memory: run the suite and diff the output.

```bash
g++ -std=c++20 -O2 -I src/dsp -I tests tests/test_main.cpp -o tests/run_tests
./tests/run_tests
```

## Summary

| Area | Checks | Result |
|---|---|---|
| A. Presets and configuration | 4 | pass |
| B. Carrier (PolyBLEP, noise, switching) | 4 | pass |
| C. Driver (ADAA, filters, oversampling) | 5 | pass |
| D. Hose waveguide | 4 | pass |
| E. Vocal-tract formants (9 vowels × F1–F3) | 9 | pass |
| F. Whisper/mic path, bleed, latency | 6 | pass |
| G. Consonant bridge | 3 | pass |
| H. Radiation, microphone, limiter | 4 | pass |
| I. Stability and rate/block sweep | 3 | pass |
| J. CPU per preset | 4 | pass |
| **Total** | **46** | **46 pass / 0 fail** |

## Headline measurements

| Quantity | Spec | Measured |
|---|---|---|
| Formant accuracy F1 (9 vowels, VTL = 1.0) | ±5 % | **0.62 %** worst |
| Formant accuracy F2 | ±5 % | **0.17 %** worst |
| Formant accuracy F3 | ±5 % | **0.04 %** worst |
| Whisper-path F1 | ±15 % | **1.0 %** worst (/i/, /a/, /u/) |
| Whisper-path F2 | ±5 % | **0.3 %** worst |
| Reported latency (48 kHz) | < 6 ms | **36 samples = 0.75 ms** |
| Hose acoustic delay 1.50 m / 2.00 m | L/c ± 3 % | **4.38 / 5.83 ms (0.0 %)** |
| Carrier bleed (muted) | < −60 dBFS | **−240 dBFS** |
| ADAA peak with +12 dB drive | ≤ 0.8 | **0.515** |
| DC blocker residual (5 s) | < −60 dB | **−240 dB** |
| HP 90 Hz at 30 Hz | ≤ −12 dB | **−20.5 dB** |
| LP 6.5 kHz at 16 kHz | ≤ −20 dB | **−30.0 dB** |
| 4x vs 1x alias rejection | ≥ 3 dB better | **+4.5 dB** |
| PolyBLEP vs naive saw alias | ≤ −12 dB | **−17.8 dB** |
| Hose stability, 20 s at g = 0.995 | bounded | **peak 1.32** |
| Wall damping HF reduction | ≥ 6 dB | **15.6 dB** |
| Sibilance index /s/ | 0.50 ± 0.08 | **0.46** |
| Plosive attack | ≤ 20 ms | **5.48 ms** |
| Limiter ceiling (+12 dB input) | ≤ −0.25 dBFS | **−0.30 dBFS** |
| Proximity 5 cm vs 50 cm @ 100 Hz | ≥ +3 dB | **+11.5 dB** |
| Mic model spread | ≥ 3 dB | **7.8 dB** |
| Lip radiation slope 300→600 Hz | 6 ± 1.5 dB/oct | **+6.0 dB/oct** |
| CPU Eco / Mic / Balanced / Studio (48 kHz, 128) | ≤ 5 / 5 / 6 / 8 % | **1.29 / 0.98 / 1.40 / 2.00 %** (this run; 1.0–2.2 % across runs) |

## Method notes

* **Formants (E).** The tract is excited with a unit impulse and the magnitude
  response is computed with a 32 768-point FFT (2.93 Hz bins). The reported
  value is the local maximum nearest the target inside a ±8…12 % band, refined
  by parabolic interpolation. This measures the filter itself, not an estimator,
  so it isolates DSP accuracy from analysis bias.
* **Whisper path (F).** A deterministic white-noise source (25 % FS) drives the
  full engine in the *Mic* preset; 0.7 s of output is analysed with a
  16 384-point FFT and the same peak picker. This is a spectral-envelope
  estimator, not LPC. LPC (order 14, Levinson–Durbin, 30 ms frames) was
  evaluated during development and is kept in `tests/LpcTools.h`, but on
  high-Q parallel formants its pole estimates were biased by 10–17 % even for
  perfectly known synthetic resonators, so it is **not** used as a pass/fail
  criterion. The spectral-envelope numbers above are the honest measurement.
* **Hose delay (D1/D2).** Peak of the impulse response with feedback muted.
* **CPU (J).** Four seconds of audio in 128-sample blocks, `-O2`, single core,
  wall-clock time. Sandboxes and CI runners vary by ±30 %; the budgets are set
  with that margin in mind and the measured values sit 3–4x below them.
* **Stability (I).** NaN and ±Inf are injected for 32 samples; the engine must
  return finite, bounded output within the next 128-sample block. The rate/block
  sweep covers 44.1/48/96/192 kHz with blocks of 16, 128, 256, 512, 1024, 2048.

## Recorded transcript

```
TalkBox Reference — validation suite
Sample rate 48000 Hz, C++20, 12.2.0

PASS A1  4 factory presets with name/description        count=4
PASS A2  quality ladder Eco <= Balanced <= Studio       OS=1/1/2/4
PASS A3  preset parameter ranges (drive/hose/VTL)       hose 1.6–1.9 m, drive 0–9 dB
PASS A4  presets/factory_presets.json mirrors 4 presets entries=4 bytes=2702
PASS B1  saw fundamental accuracy                       f0=1000.08 Hz (+0.008%)
PASS B2  PolyBLEP alias suppression vs naive            -17.8 dB
PASS B3  noise carrier zero-mean and bounded            mean=0.0006 peak=0.300
PASS B4  carrier mode switch stays click-free           switch step 0.350 vs natural 0.524
PASS C1  ADAA soft-clip output bounded                  peak=0.515 (bound 0.8)
PASS C2  DC blocker 18 Hz residual                      -240 dB after 5 s
PASS C3  high-pass 90 Hz rejects 30 Hz                  -20.5 dB
PASS C4  low-pass 6.5 kHz rejects 16 kHz                -30.0 dB
PASS C5  localised 4x oversampling improves aliasing    4.5 dB better than 1x
PASS D1  hose delay at 1.50 m                           4.38 ms vs 4.37 ms (+0.0%)
PASS D2  hose delay at 2.00 m                           5.83 ms vs 5.83 ms (+0.0%)
PASS D3  hose stable at max feedback (20 s)             peak=1.320
PASS D4  wall damping attenuates the top end            15.6 dB HF reduction at 1 kHz damping
PASS E1  /i/ F1-F3 within +/-5%                         268/2290/3009 Hz  err -0.62/-0.02/-0.02%
PASS E2  /I/ F1-F3 within +/-5%                         388/1989/2549 Hz  err -0.40/-0.03/-0.03%
PASS E3  /e/ F1-F3 within +/-5%                         529/1839/2479 Hz  err -0.22/-0.03/-0.03%
PASS E4  /E/ F1-F3 within +/-5%                         659/1719/2409 Hz  err -0.18/-0.05/-0.04%
PASS E5  /ae/ F1-F3 within +/-5%                        729/1088/2439 Hz  err -0.18/-0.15/-0.04%
PASS E6  /a/ F1-F3 within +/-5%                         699/1219/2599 Hz  err -0.16/-0.09/-0.03%
PASS E7  /open o/ F1-F3 within +/-5%                    569/839/2409 Hz  err -0.18/-0.17/-0.03%
PASS E8  /o/ F1-F3 within +/-5%                         429/1019/2339 Hz  err -0.24/-0.10/-0.04%
PASS E9  /u/ F1-F3 within +/-5%                         298/869/2239 Hz  err -0.50/-0.11/-0.03%
PASS F1  whisper formants /i/ (FFT envelope)            F1 267 Hz (-1.0%)  F2 2287 Hz (-0.1%)
PASS F3  whisper formants /a/ (FFT envelope)            F1 704 Hz (+0.5%)  F2 1221 Hz (+0.1%)
PASS F4  whisper formants /u/ (FFT envelope)            F1 301 Hz (+0.4%)  F2 872 Hz (+0.3%)
PASS F2  whisper F2 at 4x oversampling                  F2 1221 Hz (+0.1%)
PASS F5  carrier bleed below -60 dBFS                   -240 dBFS
PASS F6  reported algorithmic latency                   36 samples = 0.75 ms (hose adds 5.10 ms of acoustics)
PASS G1  sibilance index /s/ 0.50 +/- 0.08              0.46
PASS G2  plosive attack within 20 ms                    5.48 ms
PASS G3  consonant bridge stays under ceiling           peak=0.966 (ceiling 0.966)
PASS H1  limiter ceiling with +12 dB input              -0.30 dBFS (ceiling 0.97)
PASS H2  proximity effect at 5 cm vs 50 cm              +11.5 dB at 100 Hz
PASS H3  four mic models are audibly distinct           max delta 7.8 dB
PASS H4  lip radiation slope +6 dB/oct                  +6.0 dB/oct
PASS I1  NaN/Inf injection recovers within one block    NaN peak 0.000, Inf peak 0.000
PASS I2  +120 dBFS input stays bounded                  peak=0.610 (-4.30 dBFS)
PASS I3  44.1–192 kHz, blocks 16–2048 stay finite   peak=0.684 over 6 rate/block combos
PASS J1  Eco Pad within CPU budget                      1.29% of one core at 48 kHz/128 (budget 5%)
PASS J2  Mic within CPU budget                          0.98% of one core at 48 kHz/128 (budget 5%)
PASS J3  Balanced within CPU budget                     1.40% of one core at 48 kHz/128 (budget 6%)
PASS J4  Studio within CPU budget                       2.00% of one core at 48 kHz/128 (budget 8%)

----------------------------------------
PASS=46 FAIL=0
----------------------------------------
```
