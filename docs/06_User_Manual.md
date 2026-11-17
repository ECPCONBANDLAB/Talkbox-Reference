# TalkBox Reference — 06 User Manual

**Version 1.0.0 — 100% offline, no accounts, no telemetry.**

---

## Quick start (30 seconds)

1. **Load the plugin** on an instrument track (guitar, synth, or keys). Set **Carrier = Internal Talkbox Synth** if you have no external carrier — play MIDI/computer keyboard immediately.
2. **Drag the 2D Vowel Pad** (center) to morph vowels: front-top = "ee", back-bottom = "oo", center = "uh". Sound is instant.
3. **Turn knobs:** Carrier Drive (bite), Tube Character (brightness), Mouth Size (VTL: smaller = brighter/childlike, larger = darker/monster), Dry/Wet (blend).
4. **Quality:** leave on **Balanced** (default). Eco for max instances, Studio for detail.

## How to Play With Your Microphone (DAW Sidechain & Standalone Quick-Start)

### DAW Sidechain (VST3/AU)

1. Create **two tracks**: Track 1 = carrier (guitar/synth MIDI). Track 2 = mic (USB/XLR interface, input = your mic).
2. Load **TalkBox Reference** on Track 1. Set **Source = Live Mic Control**, **Carrier = External Audio** (or Internal Synth for MIDI jamming).
3. **Route mic to sidechain:** in your DAW, set Track 2 output → TalkBox sidechain input (or use **Mic Routing = L-R Split** if your DAW has tricky sidechains: L=carrier, R=mic on same track).
4. **Mic Quick-Setup banner** appears: check Mic Input Level (green when speaking), click **Auto-Gate** to calibrate, set **Consonant Crispness** ~0.7.
5. **Play:** talk or whisper into the mic — the carrier talks! Mouth words without singing (whisper works best for pure talkbox, no pitch bleed).
6. **Headphones required** (see feedback guide below).

### Standalone App

1. Launch **TalkBox Reference** standalone. Select **Audio Device** (your interface) and **Mic Input** (your USB mic).
2. Set **Source = Live Mic Control**, **Carrier = Internal Talkbox Synth**.
3. Play MIDI/computer keyboard (on-screen mini keyboard) while talking/whispering into mic. Instant funk lead.

### Whisper vs Voice tips

- **Whisper** = pure talkbox (no vocal pitch bleed, like real hardware mouthing). Best for intelligible lyrics. Boost **Whisper Boost** to 1.5–2.0 if whispering quietly.
- **Voice/Sing** = adds musical pitch to vowels (tracked formants still follow your mouth, but your voice may bleed slightly if Voice Bleed >0). Keep **Voice Bleed = 0%** for clean talkbox.
- **Gate:** if gate chatters, raise **Gate Threshold** slightly or click Auto-Gate. **Gate Behavior:** *Close Mouth Naturally* (authentic), *Hold Last Vowel* (sustains), *Return to Pad* (falls back to pad).

### Headphone feedback guide

- **Always use headphones** when mic is live. Speakers + mic = feedback loop (howling).
- Keep mic 3–5 cm from mouth (proximity warmth), angled slightly off-axis to avoid plosive pops.
- If feedback squeals: lower **Mic Pre-Gain**, move mic closer (less gain needed), increase **Mic Distance** setting (less proximity boost), or mute speakers.

### Carrier tone tips

- **Guitar:** clean neck pickup + moderate Carrier Drive (0.6) = classic talkbox. Humbuckers + higher drive = Roger Troutman funk.
- **Synth lead:** Saw + Glide 0.06 + Vibrato 0.1 = Minimoog talkbox. Pulse + higher Horn Cut (7 kHz) = DX-style bright.
- **Keys/organ:** Pulse + lower drive = mellow. Add Nasal 0.2 for nasal "ng" color.
- **Tube Character:** lower = darker/mellow, higher = brighter/crisper. **Mouth Size (VTL):** 0.85 = childlike, 1.0 = adult male, 1.2 = monster.

## Presets

- **Mic-Ready Funk Lead:** internal synth + mic control, ready to jam.
- **Whisper Talkbox:** high whisper boost + crispness, intelligible lyrics.
- **Classic Guitar Hose:** external guitar + pad, rock talkbox.
- **Pad Vowel Morphs:** internal pulse synth + pad morphing, experimental.

Import/Export JSON/XML via Preset Browser (corruption-safe: invalid files are rejected with backup, never crash).

## Troubleshooting

- **No sound:** check Carrier selector (External needs audio on track), Bypass off, Output Gain >0, Dry/Wet >0.
- **Mic not tracking:** check routing (Sidechain/L-R Split), Mic Input Level green, Gate Threshold not too high, Whisper Boost up if whispering.
- **Clicks/pops:** lower Glide (faster) or raise Glide (smoother)? Try Articulation Smooth 0.5. Check for DC offset (input gain too high).
- **CPU high:** switch to Eco, reduce instances, increase buffer to 256.

## Privacy

100% offline. No telemetry, no accounts, no background services. Uninstall in 10 seconds (see Install Guide). Portable mode: place `PortableData/` next to plugin to store presets locally with zero OS writes.
