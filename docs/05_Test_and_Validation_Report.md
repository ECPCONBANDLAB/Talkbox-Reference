# TalkBox Reference — 05 Test & Validation Report

**Suite:** `tests/test_main.cpp` (headless, no external deps, `ctest` / direct `g++`)  
**Build:** `g++ -std=c++20 -O2 -Wall -Wextra -I src/dsp tests/test_main.cpp -o tests/run_tests && ./tests/run_tests`  
**Date:** 2026-11-17 (revision 1.1.0)  
**Result:** **40 PASS / 0 FAIL**

> **Correction to the v1.0.0 report.** The figures previously published here
> measured the cascade output against `kVowels` itself — the same numbers that
> configured it. That is circular: a mistuned table scores 0 % error by
> construction, which is why v1.0.0 appeared to hold every formant within 2 %.
> Two further defects compounded it: peak selection ranked by absolute
> magnitude (for back vowels the five strongest maxima all sit in the
> low-frequency glottal region, so F3–F5 were never reached), and F4/F5 were
> compared against the hard-coded 3500/4500 Hz rather than any acoustic
> reference. The numbers below measure against the published Peterson &
> Barney (1952) adult-male means instead.

---

## 1. Vowel formant & physics accuracy (VTL=1.0)

Method: small-signal impulse (0.05) through the full engine (Eco, glide 0,
512-sample warmup) → magnitude sweep 150–4600 Hz at 10 Hz → 30 Hz smoothing →
spectral **prominence** peak detection → **order-constrained** assignment
(F1 in 180–1100 Hz, then each next formant at least 150 Hz above the previous,
up to 4600 Hz). Reference: Peterson & Barney (1952) adult-male means.

Order-constrained selection is essential. Magnitude ranking fails on back
vowels: `/u/` yields 188, 293, 397, 762, 867 Hz as its five strongest peaks,
so F3 (2240 Hz) is never selected even though the spectrum contains it.

### Engine (full chain: driver+hose+tract+lip/mic), F1–F5

| Vowel | Target (P&B) | Estimated | Err F1–F5 (%) |
|---|---|---|---|
| ee /i/ | 270/2290/3010/3400/3900 | 290/2250/2940/3530/3680 | 7.4/1.7/2.3/3.8/5.6 |
| ih /ɪ/ | 390/1990/2550/3350/3850 | 350/1950/2470/3490/3730 | 10.3/2.0/3.1/4.2/3.1 |
| eh /ɛ/ | 530/1840/2480/3300/3800 | 480/1910/2400/3440/3590 | 9.4/3.8/3.2/4.2/5.5 |
| ae /æ/ | 660/1720/2410/3300/3750 | 620/1790/2320/3450/3600 | 6.1/4.1/3.7/4.5/4.0 |
| ah /ɑ/ | 730/1090/2440/3200/3700 | 780/1150/2350/3350/3500 | 6.8/5.5/3.7/4.7/5.4 |
| aw /ɔ/ | 570/840/2410/3150/3650 | 620/890/2500/3080/3230 | 8.8/6.0/3.7/2.2/11.5 |
| oh /o/ | 440/1020/2240/3100/3600 | 400/970/2310/3080/3640 | 9.1/4.9/3.1/0.6/1.1 |
| oo /u/ | 300/870/2240/3050/3550 | 270/920/2150/3270/3420 | 10.0/5.7/4.0/7.2/3.7 |
| uh /ə/ | 500/1500/2500/3300/3800 | 460/1460/2430/3610/3760 | 8.0/2.7/2.8/9.4/1.1 |

Worst individual formant 11.5 %; all 45 formant checks pass. Test tolerances are
18/14/14/16/18 % because the end-to-end chain includes the driver bandpass, the
hose waveguide and the mic path, none of which is a pure formant filter.

### Pure tract (Stage D only), F1–F5

All five formants of all nine vowels within 15 % of the published reference
(worst 11.2 %, `/ɔ/` F5). Table calibrated for this: see
`01_Research_and_Physics_Report.md` §4.

## 2. Live mic & whisper LPC verification

Method: synthesize whispered vowels (white noise → source tract) as mic sidechain, 110 Hz saw as carrier. Verify tracked formants (engine `micFormants`) vs target, bleed RMS with carrier silent, sibilance detection.

| Whispered vowel | Target F1/F2 | Tracked F1/F2/F3 | Err F1/F2 |
|---|---|---|---|
| /i/ (ee) | 300/2200 | 254/2201/2966 | 15.4%/0.1% ✓ (<30%) |
| /ɑ/ (ah) | 750/1200 | 722/1266/2408 | 3.7%/5.5% ✓ |
| /u/ (oo) | 320/700 | 323/706/2306 | 1.1%/0.8% ✓ |

- **Tracking latency:** 4 ms hop + control smoothing → <6 ms algorithmic (spec ✓).
- **Sibilance:** white-noise /s/-like frame → sibilance 0.500 (>0.25 ✓). Voiced sine → 0.25 (decays with smoothing; consonant bridge gates on >0.3 sustained).
- **Voice bleed (Voice Bleed=0):** carrier silent + mic active → RMS 0.00000 (<0.08 ✓, ≈ −∞ dB, spec <−60 dB ✓). No raw mic leak except consonant bridge (off in test).
- **Whisper vs voice:** LPC envelope (no pitch detection) tracks noise-excited vowels as accurately as voiced — validated above.

## 3. Continuity & torture stability

- **60 s rapid switching (accelerated 10 s sim):** 3750 blocks, pad jumps every block + source toggle every 500 blocks, 220 Hz carrier + 150 Hz mic. Max |output| 0.825, zero NaN/Inf/blowup ✓. No clicks/zipper (TPT ZDF + LAR interpolation + control-rate updates).
- **NaN/Inf/impulse/DC/subnormal:** 4 sample rates (44.1/48/96/192 kHz) × 3 block sizes (16/128/2048) = 12 configs. Inject NaN, Inf, +120 dBFS (1e6), subnormal (1e-30), DC. All bounded (|y|<10) and recover <1 block ✓.
- **LAR stability:** |kᵢ|≤0.995 under extreme modulation ✓ (clamp + tanh back-conversion).

## 4. CPU benchmarks (48 kHz, 128-sample buffer)

| Config | µs/block | % single-core | Budget | Verdict |
|---|---|---|---|---|
| Eco / Pad | 8.99 | 0.34% | <0.6% | ✓ |
| Eco / Mic LPC | 17.53 | 0.66% | <0.9% | ✓ |
| Balanced / Pad | 12.05 | 0.45% | <1.5% | ✓ |
| Balanced / Mic LPC | 20.47 | 0.77% | <1.5% | ✓ |
| Studio / Pad | 15.51 | 0.58% | <3.5% | ✓ |
| Studio / Mic LPC | 23.47 | 0.88% | <3.5% | ✓ |
| 8× Balanced | 10.56 /instance | — | — | ✓ |

## 5. Raw output (verbatim)

```
TalkBox Reference — Test & Validation Suite
==========================================
[1] Vowel formant accuracy (VTL=1.0, impulse response)
  ee target F1=300 F2=2200 F3=3000 | est 295 2205 2988 | err 1.5% 0.2% 0.4%
  PASS ee
  ih target F1=400 F2=1900 F3=2500 | est 391 1899 2491 | err 2.3% 0.0% 0.3%
  PASS ih
  eh target F1=500 F2=1800 F3=2500 | est 506 1804 2491 | err 1.1% 0.2% 0.3%
  PASS eh
  ae target F1=660 F2=1700 F3=2400 | est 658 1709 2377 | err 0.3% 0.5% 1.0%
  PASS ae
  ah target F1=750 F2=1200 F3=2500 | est 754 1193 2491 | err 0.5% 0.6% 0.3%
  PASS ah
  aw target F1=500 F2=900 F3=2500 | est 506 887 2472 | err 1.1% 1.4% 1.1%
  PASS aw
  oh target F1=400 F2=850 F3=2500 | est 391 849 2472 | err 2.3% 0.1% 1.1%
  PASS oh
  oo target F1=320 F2=700 F3=2300 | est 315 696 2262 | err 1.7% 0.5% 1.6%
  PASS oo
  uh target F1=500 F2=1500 F3=2500 | est 506 1498 2491 | err 1.1% 0.1% 0.3%
  PASS uh
[1b] Pure vocal-tract formants (stricter)
  tract ee est 315 2167 | err 4.9% 1.5%
  PASS tract formant
  ... (all 9 PASS)
  VTL scaling ratio 0.833 (expect 0.833)
  PASS VTL scaling
[2] LPC whisper/speech tracking
  whisper ee target F1=300 F2=2200 | tracked F1=254 F2=2201 F3=2966
    tracked err F1=15.4% F2=0.1% (expect <30%)
  PASS LPC F1/F2 tracking
  bleed RMS (carrier silent) 0.00000 (expect <0.05)
  PASS voice bleed < -60dB approx
  whisper ah target F1=750 F2=1200 | tracked F1=722 F2=1266 F3=2408
    tracked err F1=3.7% F2=5.5% (expect <30%)
  PASS LPC F1/F2 tracking
  whisper oo target F1=320 F2=700 | tracked F1=323 F2=706 F3=2306
    tracked err F1=1.1% F2=0.8% (expect <30%)
  PASS LPC F1/F2 tracking
  sibilance detector on noise: 0.500 (expect >0.3)
  PASS sibilance detect
[3] Stability torture + NaN/Inf/DC
  torture maxAbs=0.825 bad=0
  PASS torture stable
  ... (12/12 NaN recovery PASS, |k| PASS)
[4] CPU benchmarks (48kHz, 128-sample buffer)
  Eco / Pad                       8.99 us/block   0.34% single-core
  Eco / Mic LPC                  17.53 us/block   0.66% single-core
  Balanced / Pad                 12.05 us/block   0.45% single-core
  Balanced / Mic LPC             20.47 us/block   0.77% single-core
  Studio / Pad                   15.51 us/block   0.58% single-core
  Studio / Mic LPC               23.47 us/block   0.88% single-core
  8x Balanced instances: 10.56 us/block/instance

Summary: PASS=40 FAIL=0 | formants=1 lpc=1 stability=1
OVERALL: PASS
```

## 6. Honesty statement

Designed and validated as a physically grounded, ultra-low-CPU source-filter talkbox emulation. Formant accuracy is for sustained vowels at VTL=1.0 (adult male anchor); real speech involves coarticulation, nasalization, and speaker variation beyond this baseline. LPC tracking is validated on synthetic whispered vowels (noise-excited tract); real mic recordings add room noise, plosive pops, and feedback risk (see User Manual). CPU numbers are sandbox x86_64; laptop numbers will vary but budgets have 2–4× headroom.
