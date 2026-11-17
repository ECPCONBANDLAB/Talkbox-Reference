# 07 — Limitations, known issues and roadmap

## Honest limitations

1. **Sustained vowels, not coarticulation.** The tract is static per vowel with
  no interpolation between vowel targets (the UI switches, it does not glide).
  Real speech moves formants continuously; automating *Vowel* steps the
  coefficients at block boundaries.
2. **The formant measurements are filter measurements.** The ±0.6 % figures in
  `03-validation.md` describe the DSP's magnitude response to an impulse. They
  do not claim that a listener perceives the vowel identically to a human
  speaker — only that the resonances are where the table says they are.
3. **Whisper validation uses a spectral-envelope estimator, not LPC.** An LPC
  order-14 analysis was implemented (`tests/LpcTools.h`) and tested against
  synthetic AR processes; on high-Q parallel formants its pole estimates were
  biased by 10–17 %, so it was excluded from the pass/fail criteria rather than
  tuned until it agreed with the design targets.
4. **The microphone models are static and minimum-phase.** No distance-dependent
  air absorption, no off-axis response, no room. Proximity is a fixed low shelf
  scaled by distance.
5. **Consonants are gestures, not phonemes.** P/S/N are noise-burst envelopes
  triggered by the UI. There is no text-to-speech, no pitch-synchronous
  analysis, and no formant tracking of an input voice.
6. **No AAX.** Avid's SDK licence forbids redistribution in this form.
7. **CPU numbers are sandbox numbers.** They are consistent across runs on the
  same machine, but do not treat 1.03 % as a promise for your laptop.
8. **JUCE shell is not built in CI on every platform.** CI builds VST3 +
  Standalone on Linux and runs the DSP suite on the three platforms; there is
  no signed macOS/Windows binary in this repository.

## Known issues

| ID | Severity | Description | Workaround |
|---|---|---|---|
| TB-001 | low | Automating *Oversampling* while audio runs resets the driver and can click | Set it once, before playback |
| TB-002 | low | Vowel changes can click if a consonant burst is active | Trigger consonants between vowel changes |
| TB-003 | low | At VTL > 1.35 with /i/, F3 moves above 4 kHz and the peak picker in the tests is out of range | Documented; the engine is fine, only the test band is narrow |
| TB-004 | cosmetic | The peak meter has no peak-hold or dB scale | Planned |

## Roadmap

**1.1**
* Vowel glide (one-pole interpolation of formant frequencies and bandwidths).
* Peak-hold meter and a small spectrum display fed by the existing `tb::Fft`.
* CLAP parameter modulation for *Vowel* so hosts can ratchet it.

**1.2**
* Second-order ADAA for the driver (further alias suppression at 1x).
* Distance-dependent HF air absorption.
* IR export of the engine's impulse response for convolution-based hosts.

**2.0**
* Optional pitch-tracked carrier from the input (so a guitar's own pitch drives
  the internal oscillator).
* Per-vowel articulation envelopes.
* Signed installers for Windows and macOS via GitHub Actions.

## Contributing

The contract before a PR is merged:

1. `./tests/run_tests` must print `PASS=46 FAIL=0`.
2. New DSP must keep the "no allocation, no locks" rule.
3. New measurements go into `tests/test_main.cpp`, not into prose.
4. Documentation numbers are copied from `artifacts/test_output.txt`.
