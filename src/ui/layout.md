# TalkBox Reference — UI layout spec

Fixed aspect 720 × 460 (resizable to 1080 × 690 by scaling the same grid).

| Region | Rect (x, y, w, h) | Content |
|---|---|---|
| Header | 24, 0, 672, 64 | Wordmark + subtitle, preset selector (48 px label + 200 px box) |
| Panel A | 16, 74, 220, 330 | **Source**: vowel, carrier mode, pitch, noise, hose length, feedback |
| Panel B | 244, 74, 232, 330 | **Vocal tract**: VTL, formant shift, nasal, lips, drive, oversampling |
| Panel C | 484, 74, 220, 330 | **Microphone**: model, distance, saturation, output, ceiling |
| Footer | 24, 418, 680, 44 | Articulation buttons (P / S / N), peak meter, latency readout |

## Tokens

- Background `#10131A`, panels `#181D27`, control wells `#222A36`
- Accent (tract) `#41D1B0`, accent (source) `#FFB454`, accent (mic) `#8AB4FF`
- Text `#DFE6F0`, secondary text `#7C8798`
- Rotary controls: 92 × 92 hit area, 62 × 16 text box below, 11 px captions in caps
- Peak meter: 10 px rounded bar, 30 Hz refresh with 0.86 decay, red above −0.9 dBFS

## Interaction

- Every control is bound through `AudioProcessorValueTreeState`; no manual parameter plumbing.
- Preset box lists the four factory presets and is restored from the host session.
- P / S / N trigger the consonant bridge (plosive, sibilant, nasal) with a single click.
- Latency readout shows algorithmic latency and hose acoustic delay separately.
