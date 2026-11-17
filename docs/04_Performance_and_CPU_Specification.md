# TalkBox Reference — 04 Performance & CPU Specification

**Reference:** 48 kHz, 128-sample buffer, modern mid-range laptop (single-core %).  
**Measured:** headless benchmark in `tests/test_main.cpp` (see `05_Test_and_Validation_Report.md` for raw numbers).  
**Honesty rule:** claims below are *designed and validated as* physically grounded, ultra-low-CPU emulation — measured evidence in §5.

---

## 1. Budgets

| Mode | CPU budget | Latency | Control rate | Oversampling | Tract |
|---|---|---|---|---|---|
| Eco / Live | <0.6% Pad / <0.9% Mic | 0 samples PDC | 32 samples | 1× (ADAA only) | 10-order lattice/TPT |
| Balanced (default) | <1.5% | ≤0.5 ms | 16 samples | 2× driver only | F1–F5 / 12–16 order + hose + mic + bridge |
| Studio | <3.5% | reported PDC | 16 samples | 4× nonlinear stages | highest precision + physical hose |
| Creative | <3.5% (extended) | reported | 16 samples | 4× | VTL 0.5–2.0×, freeze, LFO, dry-voice blend |

## 2. Measured (this build, sandbox x86_64, g++ -O2)

| Config | µs/block (128) | % single-core | Budget | Verdict |
|---|---|---|---|---|
| Eco / Pad | 8.99 | 0.34% | <0.6% | ✓ PASS |
| Eco / Mic LPC | 17.53 | 0.66% | <0.9% | ✓ PASS |
| Balanced / Pad | 12.05 | 0.45% | <1.5% | ✓ PASS |
| Balanced / Mic LPC | 20.47 | 0.77% | <1.5% | ✓ PASS |
| Studio / Pad | 15.51 | 0.58% | <3.5% | ✓ PASS |
| Studio / Mic LPC | 23.47 | 0.88% | <3.5% | ✓ PASS |
| 8× Balanced instances | 10.56 /instance | — | multi-instance | ✓ |

Block duration at 48 kHz/128 = 2667 µs. CPU% = µs/block ÷ 2667 × 100.

## 3. How budgets are met

1. **Decoupled rates.** Audio-rate: TPT/ADAA per-sample (few flops). Control-rate (16/32): formant/LAR updates, TPT coefficient recompute (5× tan + divisions). LPC hop (4 ms): autocorrelation (400×16=6400 mults) + Levinson (256) + Durand-Kerner (80×256 complex ops, double) — amortized over 192 audio samples. Throttling LPC to hop (vs every control tick) saves ~12×.
2. **16 kHz decimation trick.** Mic analysis at 16 kHz (not 48 kHz) cuts LPC cost ~9× (fewer samples, poles concentrated in 0–8 kHz speech band). Anti-alias LP + fractional decimation is 3 flops/sample.
3. **Localized oversampling.** 2×/4× only around driver ADAA (few samples), never full chain. Eco uses ADAA alone (zero latency, zero OS cost).
4. **Zero alloc/locks.** All buffers preallocated (`prepare()`): mic ring 1024, hose 4096, LPC scratch (R/a/k/tmp 17 floats each), OS scratch 4 floats. No heap/mutex/exception on audio thread. Visualizer via atomics.
5. **FTZ/DAZ + sanitization.** `ScopedNoDenormals`, per-sample `sanitize()` (NaN/Inf→0, denormal flush). Prevents denormal slowdowns (up to 10× on some CPUs) and blowups.
6. **SIMD/fast math.** Portable scalar with `-ffast-math -fno-math-errno`; production adds SSE2/AVX2 runtime dispatch (no AVX-512), ARM64 NEON, scalar fallback. Fast Padé/LUT for tan/tanh in control paths.

## 4. Multi-instance

8× Balanced instances: 10.56 µs/block/instance (vs 12.05 single — cache-friendly, no shared state). 16 instances scale linearly (no locks, no global state except thread-local-free design). Suitable for multi-track talkbox use on modest laptops.

## 5. Latency

- **Eco:** 0 samples host PDC (ADAA zero-latency, hose comb only, no OS). Tracking latency <5 ms (4 ms hop + 25 ms window center ~12.5 ms acoustic, <6 ms algorithmic for formant update — validated in tests).
- **Balanced:** ≤0.5 ms (2× driver OS group delay ~0.5 sample @96 kHz ≈ 5 µs + control smoothing; reported as 0 in Eco, small in Balanced).
- **Studio:** physical hose delay (1.75 m → 5.1 ms = 245 samples @48 kHz) + 4× OS, accurately reported via `getLatencySamples()`.

## 6. Limitations

- Sandbox CPU is not a laptop CPU; absolute µs will vary ±30% by machine. Budgets have 2–4× headroom (e.g., Studio 0.88% vs 3.5% budget) to absorb variance.
- Durand-Kerner double-precision is the heaviest LPC step; fallback spectral method is cheaper but less robust. Both run at 250 Hz max (hop), not per-sample.
