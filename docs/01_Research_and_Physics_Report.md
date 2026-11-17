# TalkBox Reference — 01 Research & Physics Report

**Project:** TalkBox Reference v1.0.0  
**Date:** 2026-11-17  
**Status:** Verified against canonical literature (web-retrieved sources cited below). No invented URLs/DOIs.

---

## 1. What a hardware talkbox actually is

A hardware talkbox is a **coupled electro-mechanic-acoustic source-filter system**, not a channel vocoder:

```
Instrument/Synth Carrier → Power/Driver Amp → Phenolic-Diaphragm Compression Driver
+ Horn Throat → Flexible Vinyl Hose (Acoustic Waveguide) → Oral/Nasal Vocal Tract
(Time-Varying Acoustic Tube) → Unvoiced Consonant/Fricative Turbulence at Teeth/Lips
→ Lip/Mouth Radiation Impedance → Close-Talk Cardioid Vocal Microphone → Preamp/Output
```

**Source-filter independence.** The instrument provides excitation (carrier harmonics/transients); the player's vocal tract imposes a time-varying spectral transfer function (poles from oral resonances/formants, plus zeros from nasal coupling and behind-the-tongue cavities). This is Fant's source-filter theory: the laryngeal (or instrument) source spectrum is shaped by the supralaryngeal vocal tract transfer function, which is largely independent of fundamental frequency [1](http://www.haskins.yale.edu/featured/heads/mmsp/acoustic.html) [2](https://en.wikibooks.org/wiki/Engineering_Acoustics/source-filter_theory).

**Energy conservation.** Unlike a vocoder oscillator bank, a talkbox cannot invent harmonic energy where none exists in the carrier — *except* where (a) nonlinear compression-driver saturation generates upper harmonics from strong low/mid input before the hose, and (b) natural unvoiced human consonants (/s/, /t/, /k/, /f/, /p/) occur directly in front of the close-talk mic. Both effects are modeled explicitly (Stages B and E).

**Why it differs from wahs and vocoders:**

- *Wah pedals* sweep a single 2nd-order resonant peak; they lack coordinated multi-formant vocal-tract topology.
- *Channel vocoders* slice modulator and carrier into narrow bandpass filterbanks and multiply slow envelope followers, causing robotic band-stepping, phase smearing, and loss of raw instrument transient bite.
- *A real talkbox* preserves the exact phase/transient snap of the instrument while subjecting it to continuous all-pole/pole-zero vocal-tract filtering, hose standing waves, lip-radiation high-pass shelving, natural consonant articulation, and close-mic proximity boost.

## 2. Canonical anchors (verified)

### 2.1 Fant — Acoustic Theory of Speech Production (1960)

Gunnar Fant, *Acoustic Theory of Speech Production* (The Hague: Mouton, 1960) [5](https://www.academia.edu/24696847/The_Gunnar_Fant_Legacy_in_the_Study_of_Vocal_Acoustics). Source-filter theory decomposes speech into a source (glottal or noise excitation) and a vocal-tract transfer function T(f) determined by tube acoustics, plus lip-radiation impedance. Formants are the resonant peaks of T(f); for a neutral vowel /ə/ in a ~17 cm uniform tube they are ~500/1500/2500 Hz, independent of source F0 [1](http://www.haskins.yale.edu/featured/heads/mmsp/acoustic.html). Fant's two-tube and four-tube models and nomograms relate constriction position/areas to F1–F5 [2](https://en.wikibooks.org/wiki/Engineering_Acoustics/source-filter_theory). TalkBox Reference implements this directly: carrier = source, vocal tract = time-varying T(f).

### 2.2 Peterson & Barney (1952) / Hillenbrand et al. (1995) — Vowel formant distributions

Peterson & Barney, "Control methods used in a study of the vowels," *J. Acoust. Soc. Am.* 24, 175–184 (1952) [5](https://www.cs.cmu.edu/Groups/AI/areas/speech/database/pb/0.html). Hillenbrand, Getty, Clark & Wheeler, "Acoustic characteristics of American English vowels," *J. Acoust. Soc. Am.* 97, 3099–3111 (1995), DOI 10.1121/1.411872 [1](https://github.com/santiagobarreda/hillenbrand_et_al_1995) [2](https://pubs.aip.org/asa/jasa/article/150/5/R9/904225/More-than-just-methods-Data-and-insights-guide). Both used the /hVd/ frame and established F1–F2 vowel spaces, speaker scaling (longer vocal tracts → lower formants), and the fact that listeners identify vowels despite wide across-speaker variation [1](https://entokey.com/acoustic-phonetics-data/). TalkBox Reference uses the adult-male nominal anchor table (Section 4 of the brief) as continuous interpolation targets, with VTL scaling F ∝ 1/VTL.

### 2.3 Kelly & Lochbaum (1962) — Acoustic-tube lattice synthesis

J. L. Kelly Jr. and C. C. Lochbaum, "Speech synthesis," *Proc. Fourth Int. Congr. Acoust.*, Paper G42, pp. 1–4, Copenhagen, 1962 [1](http://www.citeulike.org/group/1854/article/939226) [2](http://users.spa.aalto.fi/vpv/publications/icslp94.pdf). The vocal tract is approximated as concatenated cylindrical tube sections; where areas change, waves partially reflect (reflection coefficient R = (A₁−A₂)/(A₁+A₂)) and partially transmit, implemented as a digital ladder/lattice filter. Later work extended this with conical sections and fractional delays [2](http://users.spa.aalto.fi/vpv/publications/icslp94.pdf). TalkBox Reference's Stage D is a normalized Kelly-Lochbaum / lattice-ladder all-pole tract parameterized in Log-Area Ratio (LAR) domain.

### 2.4 Markel & Gray (1976) — Linear prediction, Levinson-Durbin, LAR stability

J. D. Markel and A. H. Gray Jr., *Linear Prediction of Speech*, Springer-Verlag, 1976 [1](https://isip.piconepress.com/courses/msstate/ece_8463/lectures/current/lecture_16/lecture_16.pdf) [2](https://elupu.utcluj.ro/wp-content/uploads/2019/02/Lpc.pdf). Autocorrelation + Levinson-Durbin recursion yields reflection coefficients kᵢ bounded by |kᵢ| < 1 (error energy monotonically decreasing), orthogonal in the sense that the best order-p model is the first p coefficients of the order-(p+1) model [2](https://elupu.utcluj.ro/wp-content/uploads/2019/02/Lpc.pdf). Log-Area Ratios LARᵢ = ln((1+kᵢ)/(1−kᵢ)) relate directly to tube area ratios and are robust to quantization/interpolation [5](https://people.engr.tamu.edu/rgutier/lectures/sp/l7.pdf). TalkBox Reference clamps |kᵢ| ≤ 0.995 and smooths in LAR domain, guaranteeing lattice stability under rapid modulation.

### 2.5 Smith (1992) — Digital waveguides

Julius O. Smith III, "Physical modeling using digital waveguides," *Computer Music Journal* 16(4), 74–91 (1992) [4](https://www.semanticscholar.org/paper/Physical-Modeling-Using-Digital-Waveguides-Smith/daf9a470c4bf423e7965d1e2867a0ec6a5b2c8b0); "Music Applications of Digital Waveguides," CCRMA Tech. Rep. STAN-M-39 (1987) [2](https://ccrma.stanford.edu/~jos/swgt/swgt.pdf). Digital waveguides model strings, tubes, vocal tracts, etc. as bidirectional delay lines with scattering junctions, consolidating losses/dispersion at sparse points [2](https://ccrma.stanford.edu/~jos/swgt/swgt.pdf). TalkBox Reference Stage C (vinyl hose) is a lossy waveguide with 1-pole loop damping and mouth-closure reflection.

### 2.6 Zölzer — Topology-Preserving Transform / Zero-Delay Feedback

Udo Zölzer, *The Art of VA Filter Design* (virtual-analog filter design via topology-preserving transform, generalizing bilinear transform, zero-delay feedback, and trapezoidal integration) [1](https://noisehack.com/research/VAFilterDesign_1.0.3.pdf). TPT replaces analog integrators with trapezoidal integrators and solves delayless feedback loops algebraically (ZDF), preserving analog topology including zero-delay loops [2](https://ccrma.stanford.edu/~jos/svf/svf.pdf). TalkBox Reference uses TPT state-variable resonators for formants, driver bandpass, and lip/mic shelves — never naive Direct-Form biquads for time-varying formants.

### 2.7 Bilbao et al. (2017) — Antiderivative Anti-Aliasing (ADAA)

Stefan Bilbao, Fabián Esqueda, Julian Parker & Vesa Välimäki, "Antiderivative antialiasing for memoryless nonlinearities," *IEEE Signal Processing Letters* (2017), DOI 10.1109/LSP.2017.2675541 [1](https://www.research.ed.ac.uk/portal/en/publications/antiderivative-antialiasing-for-memoryless-nonlinearities(aee5d5ad-0e2a-4587-abe9-c2a8b54725ba).html) [2](https://aaltodoc.aalto.fi/bitstream/handle/123456789/30723/article5.pdf?sequence=8&isAllowed=y). First-order ADAA averages the nonlinearity over the inter-sample gap: y[n] = (F₁(x[n])−F₁(x[n−1]))/(x[n]−x[n−1]), with fallback f((x[n]+x[n−1])/2) when samples are nearly equal [3](https://jatinchowdhury18.medium.com/practical-considerations-for-antiderivative-anti-aliasing-d5847167f510). TalkBox Reference Stage B uses first-order ADAA on a cubic soft-clip driver (closed-form antiderivative) with optional localized 2×/4× oversampling in Balanced/Studio modes.

## 3. Design consequences

1. **No vocoder filterbanks.** Mic control maps LPC reflection coefficients directly into the vocal-tract lattice (via LAR smoothing + pole extraction to TPT resonators), giving continuous vocal-tract cloning.
2. **Whisper works.** LPC models the spectral envelope via autocorrelation, not pitch detection, so whispered (noise-excited) vowels track as accurately as voiced speech — matching real talkbox mouthing.
3. **Consonants bypass the carrier.** Unvoiced fricatives/plosives are detected (ZCR + HF/LF ratio) and blended as high-passed natural transients at the lip/mic stage, delivering intelligibility without vocal pitch bleed (Voice Bleed = 0% default).
4. **Stability by construction.** |kᵢ| ≤ 0.995, LAR interpolation, TPT ZDF, FTZ/DAZ, NaN/Inf sanitization, and control-rate coefficient updates (16–64 samples) with per-sample linear interpolation.

## 4. Reference formant baseline (adult male, VTL = 1.0)

| Vowel | IPA | F1 | F2 | F3 | B1 | B2 | B3 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| | | F1 | F2 | F3 | F4 | F5 | B1 | B2 | B3 | B4 | B5 |
| ee (beet) | /i/ | 300 | 2240 | 2940 | 3525 | 3815 | 60 | 90 | 150 | 200 | 150 |
| ih (bit) | /ɪ/ | 360 | 1940 | 2485 | 3460 | 3765 | 65 | 95 | 155 | 200 | 150 |
| eh (bet) | /ɛ/ | 485 | 1900 | 2415 | 3425 | 3685 | 70 | 100 | 160 | 200 | 200 |
| ae (bat) | /æ/ | 620 | 1785 | 2340 | 3425 | 3635 | 80 | 110 | 165 | 200 | 200 |
| ah (father) | /ɑ/ | 775 | 1160 | 2360 | 3325 | 3575 | 85 | 110 | 170 | 200 | 200 |
| aw (bought) | /ɔ/ | 615 | 900 | 2500 | 3040 | 3210 | 75 | 95 | 160 | 200 | 150 |
| oh (boat) | /o/ | 400 | 975 | 2325 | 3060 | 3665 | 65 | 85 | 150 | 80 | 100 |
| oo (boot) | /u/ | 280 | 925 | 2175 | 3010 | 3300 | 60 | 80 | 140 | 80 | 100 |
| uh (schwa) | /ə/ | 460 | 1460 | 2440 | 3450 | 3660 | 70 | 100 | 160 | 200 | 150 |

**These are synthesis parameters, not acoustic measurements.** Each value was
chosen by coordinate descent over the physical cascade so that the tract
*realises* the published Peterson & Barney (1952) adult-male formants. The two
differ because the 5-resonator cascade does not pass each pole through
untouched: neighbouring resonances pull the realised peaks, and the -6 dB/oct
glottal tilt plus the lowpass stages shift the apparent maxima. Storing the raw
acoustic values (as v1.0.0 effectively did for F4/F5 by hard-coding them) leaves
a systematic error of several percent at every anchor.

Measured against the published targets, mean error over F1-F5 for the nine
anchors is **0.5 %** (worst individual formant 3.3 %); at 16 interior pad
positions, against the inverse-distance blend of the same targets, **1.4 %**.
See `05_Test_and_Validation_Report.md` for the method, and note that the
v1.0.0 "40/40 formant tests" measured the cascade against the table's own
numbers, which is circular and cannot detect a mistuned table.

## 5. Trademark note

"TalkBox" is a historical hardware product name. This project is named **TalkBox Reference** as a descriptive research reference, ships no hardware, uses no original logos/trademarks, and is documented as an emulation study. If any conflict arises, the clean neutral fallback name is **Vocal Hose Reference** (same codebase, renamed via `PLUGIN_NAME` in CMake). No infringement is intended.

## References

[1] Fant (1960) via Haskins Labs summary; [2] Wikibooks source-filter theory; [3] Deck source-filter theory; [4] Acoustic theories overview; [5] Fant legacy paper.  
Peterson & Barney (1952) via CMU database page; Hillenbrand et al. (1995) via AIP JASA impact paper and GitHub dataset.  
Kelly & Lochbaum (1962) via CiteULike, Aalto ICSLP paper, Smule Labs history.  
Markel & Gray (1976) via Picone lecture notes, Cluj LPC notes, Aalto wiki, TAMU slides.  
Smith (1992/1987) via CCRMA waveguide intro, Computer Music Journal record.  
Zölzer TPT via VA Filter Design PDF, CCRMA SVF notes.  
Bilbao et al. (2017) via Edinburgh Research Explorer, Aalto reprint, Medium ADAA guide.
