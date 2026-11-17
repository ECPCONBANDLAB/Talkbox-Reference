# 05 — Using the plugin

## Routing

TalkBox Reference is an **insert effect** with a stereo input used as the
carrier side-chain and a stereo output.

```
guitar / synth ──► [ TalkBox Reference ] ──► amp / console / DAW
                        ▲ vowel, hose, mic
```

The left and right inputs are summed to mono for the carrier. If nothing is
connected (or the host sends silence), switch *Carrier* to `Saw`, `Square`,
`Pulse` or `Noise` and use the internal PolyBLEP oscillator at the *Pitch*
control.

## Controls

| Control | Range | What it does |
|---|---|---|
| **Preset** | 4 factory | Immediate starting points (see below) |
| **Vowel** | /i I e E ae a open-o o u/ | Selects the formant set; F1–F5 in `src/dsp/VocalTract.h` |
| **Carrier** | External, Saw, Square, Pulse, Noise | Source; `External` uses the input bus |
| **Pitch** | 40–880 Hz | Internal oscillator frequency (ignored when external) |
| **Noise** | 0–1 | Blends noise into the carrier (breath / whisper) |
| **Hose length** | 1.5–2.0 m | Acoustic delay 4.37–5.83 ms; also changes the comb colour |
| **Feedback** | 0–0.995 | Loop gain of the hose; > 0.95 starts to self-resonate |
| **VTL** | 0.70–1.45 | Scales the vocal tract; > 1 is a longer (lower) tract |
| **Formant** | ±12 st | Transposes the formants without changing the pitch |
| **Nasal** | 0–1 | Couples the nasal branch (280 Hz pole, 900 Hz zero) |
| **Lips** | 0–1 | Aperture; 0 mutes and dulls (closed mouth) |
| **Drive** | −24…+36 dB | Level into the ADAA soft clip |
| **Oversampling** | 1x / 2x / 4x | Local to the driver only; 4 samples of extra latency |
| **Microphone** | Dynamic, Small diaphragm, Condenser, Ribbon | Four colourations + proximity scaling |
| **Distance** | 0.02–1.5 m | Proximity bass lift and model scaling |
| **Saturation** | 0–1 | Post-mic tanh stage |
| **Output / Ceiling** | −24…+12 dB / −12…0 dBFS | Make-up and limiter ceiling |

Articulation buttons **P / S / N** fire the consonant bridge (plosive, sibilant,
nasal). They are one-shot gestures; automation of the consonant is intentionally
not exposed as a host parameter to keep the gesture sample-accurate.

## Factory presets

| Preset | Character | Key settings |
|---|---|---|
| **Eco Pad** | Cheapest, internal saw, soft | 1x OS, 3 dB drive, 1.60 m hose, SDC mic, −4 dB out |
| **Mic** | Microphone-first, minimal colour | 1x OS, 0 dB drive, external carrier, 10 cm distance |
| **Balanced** | Default voice | 2x OS, 6 dB drive, 1.75 m hose, dynamic, −3 dB out |
| **Studio** | Most detail and air | 4x OS, 9 dB drive, 1.90 m hose, condenser at 22 cm |

The JSON in `presets/factory_presets.json` mirrors the table in
`src/dsp/Engine.h` (test A4 guards the two from drifting apart).

## Recipes

**Classic 70s talkbox.** External guitar, *Drive* 9–12 dB, hose 1.75 m,
*Vowel* /a/ or /o/, condenser at 15 cm, output −4 dB. Automate *Vowel* between
/a/ and /u/ for the "wow" vowel movement.

**Whisper layer.** Carrier `Noise`, *Noise* 1.0, *Drive* 0 dB, *Vowel* /i/,
*Lips* 0.7, output −8 dB. The whisper-path tests (F1–F4) measure exactly this
configuration.

**Bass talkbox.** VTL 1.25, *Formant* −5 st, hose 1.90 m, drive 3 dB — the
formants drop without detuning the carrier.

**Self-resonating hose.** Feedback 0.97–0.99, hose 2.0 m, drive 0 dB. It is
bounded by design (test D3), but it will ring for several seconds.

## Host automation tips

* Automate *Vowel* as a discrete parameter; steps between vowels are click-free
  because the formant coefficients are recomputed per block and the resonators
  keep their state.
* Do not automate *Oversampling*: it rebuilds the FIR pair and resets the driver.
  Use it as a setup switch.
* The dry/wet of the effect is implicit — the plugin output is fully wet. Put a
  mixer or a parallel bus after it if you want to blend the raw carrier.
