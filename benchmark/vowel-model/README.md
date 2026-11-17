# Vowel-model measurement tools (v1.1.0)

Reproducible instruments behind the figures in `docs/01` section 4 and
`docs/05` section 1. Every tool links the real engine in `src/dsp/`; nothing is
simulated.

Both the previous table (`git show 656d0ba:src/dsp/TbVocalTract.h`) and the
current one can be measured with the *same* instrument, which is the point: the
v1.0.0 figures came from an instrument that measured the cascade against its own
configuration, so they were circular.

## Why order-constrained peak picking

Ranking spectral maxima by magnitude does not work for back vowels. Sweeping
`/u/` with the shipped table returns these five strongest maxima:

    188  293  397  762  867 Hz

All five are glottal and low-frequency structure. F3 sits at 2240 Hz and is
never reached, so a magnitude-ranked instrument reports a ~70 % F3 error that
says more about the instrument than about the model.

`measure_formants.cpp` therefore searches F1 in 180-1100 Hz, places each
subsequent formant at least 150 Hz above the previous one, and validates a peak
by its *prominence* (height above the higher of its two flanking minima) rather
than by its height.

## Tools

| file | purpose |
|---|---|
| `measure_formants.cpp` | Realised F1-F5 per anchor vs Peterson & Barney (1952). Deterministic sine sweep through the vocal tract: no windowing artefacts, no random ripple. |
| `measure_interpolation.cpp` | Same measurement at 16 interior pad positions, against the inverse-distance blend of the published targets. Catches tables that are right at anchors and wrong between them. |
| `calibrate_anchors.cpp` | Coordinate descent over the physical cascade. Finds the F1-F5 that make the tract *realise* the published formants. This produced the shipped values. |

## Why the shipped values differ from the acoustic targets

`kVowels` stores **synthesis parameters**, not acoustic measurements. The
5-resonator cascade does not pass every pole through untouched: neighbouring
resonances pull the realised peaks, the -6 dB/oct source tilt moves the apparent
maxima, and the lowpass stages attenuate the upper resonances. Storing the raw
published values leaves a systematic error at every anchor, which is exactly
what the old table did for F4/F5 where it hard-coded 3500/4500 Hz regardless of
vowel.

`calibrate_anchors.cpp` closes that loop. It is not curve-fitting to a flawed
measurement: the objective is the realised peak frequency of the real filter.

## Build

Each tool needs the engine headers on the include path:

    g++ -std=c++20 -O2 -I src/dsp benchmark/vowel-model/measure_formants.cpp -o /tmp/mf
    /tmp/mf

`measure_formants.cpp` and `measure_interpolation.cpp` read the anchor table
through `kVowelPadXY` / `kVowels`, so they run unmodified against either
version; swap `TbVocalTract.h` to compare.

## Figures produced (v1.1.0)

| | v1.0.0 table | v1.1.0 table |
|---|---|---|
| anchor error, mean F1-F5 | 3.8 % | **0.5 %** |
| worst anchor formant | 45.1 % | 3.3 % |
| interpolation, 16 positions | 3.4 % | **1.4 %** |
| table size | 432 B | **288 B** |
| `sizeof(TalkboxEngine)` | 25472 B | 25472 B |
| CPU, Balanced+Mic (median of 5) | 24.08 us | 24.04 us |

CPU is unchanged within run-to-run noise and is not claimed as an improvement.
Single runs of the same binary span 20.0-28.0 us, so any claim resting on one
run is noise.
