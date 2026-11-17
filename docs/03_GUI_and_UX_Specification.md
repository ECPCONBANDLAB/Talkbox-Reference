# TalkBox Reference — 03 GUI & UX Specification

**Design:** responsive, high-contrast, GPU-optional, DPI-scalable vector interface. Near-zero CPU when open (30 Hz visualizer timer), 0% when closed (editor destroyed). No bitmap assets (all vector-drawn).

---

## Top header bar

- **Source Mode Pill:** `Pad/MIDI` vs `Live Mic Control` (segmented control, keyboard shortcut M).
- **Carrier Selector:** `External Audio` vs `Internal Talkbox Synth` (segmented, shortcut C).
- **Preset Browser:** search field, tag filter, favorites (star), Import/Export JSON/XML with corruption safety (schema version check, backup on parse fail, atomic write).
- **A/B Compare:** two slots, instant toggle (shortcut A), per-slot undo.
- **Undo/Redo:** parameter history (50 steps, message-thread only).
- **Simple / Advanced Toggle:** switches between Simple page and tabbed Advanced (shortcut Tab).
- **Quality Selector:** `Eco` / `Balanced` / `Studio` / `Creative` (segmented, shows latency badge: Eco 0 samples, Balanced ≤0.5 ms, Studio reported PDC).
- **CPU/Latency Badge:** live µs/block + % (from DSP timer, 4 Hz update, no audio-thread locks).
- **Bypass:** toggle (shortcut B), true bypass (dry carrier through DC blocker only, no tract).

## Simple view (zero-friction)

- **Central 2D Vowel & Mouth Visualizer** (`VowelPad` component): drag in Pad Mode; in Mic Mode animates live to tracked vowels/mouth openness (F1/F2 → pad coords, mouth arc from gate). Vowel anchors labeled (ee/ih/eh/ae/ah/aw/oh/oo/uh). High-contrast (black bg, orange cursor, cyan mouth arc).
- **Mic Quick-Setup & Headphone Safety banner** (appears when Mic Mode enabled, dismissible): Mic Input Level meter, Auto-Gate calibration button (measures noise floor, sets threshold), Whisper/Speech sensitivity slider, Consonant Crispness knob, Headphone Feedback Warning (icon + text: "Use headphones — mic + speakers = feedback").
- **Core Macro Knobs (8):** Carrier Drive, Tube Character, Mouth Size (VTL), Articulation Smooth, Consonant Crispness, Mic Proximity, Dry/Wet Mix, Output Gain. Large hit targets (48 px), drag vertical, scroll fine, double-click reset.

## Advanced tabbed views

### 1. Vocal Tract
XY Pad (same as Simple, larger) + Jaw, Tongue X, Tongue Height, Lip Rounding, Mouth Opening, Nasal Coupling, VTL Scale (0.5–2.0 in Creative, 0.75–1.35 nominal), Glide/Inertia, live F1–F5 / LPC spectral envelope curve (vector-drawn, 30 Hz).

### 2. Mic & Carrier
Mic Routing (Sidechain / L-R Split / Standalone Mic), Mic Pre-Gain, Noise Gate, Gate Release Behavior (Close Mouth / Hold Vowel / Return to Pad), Whisper Boost, Consonant Crispness, Sibilance De-Ess, Voice Bleed (default 0%), Internal Synth (Waveform Saw/Pulse, Octave, Glide, Vibrato, MIDI Channel).

### 3. Hardware Chain
Driver Comp/Drive, Diaphragm Asymmetry, Horn High-Cut (5.5–7.5 kHz), Hose Length (1.5–2.0 m), Hose Damping, Mic Model (57/58, Condenser, Ribbon, Flat), Mic Distance (1–30 cm), Proximity Boost, Studio Ambience (default 0%).

### 4. System & Uninstall Help
Oversampling/Quality details (per-mode latency/CPU budget table), MIDI Learn matrix (click-learn, per-parameter), real-time DSP timer (µs / %), User Manual link (opens `docs/06_User_Manual.md`), Clean Uninstall / Data Folder Locator (shows exact paths, opens folder, launches uninstaller).

## Accessibility & DPI

- All controls keyboard-focusable, visible focus ring, screen-reader labels.
- DPI scaling via JUCE `Desktop::getGlobalScaleFactor` (vector redraw, no bitmap blur).
- Color-contrast: white on darkslategrey (≥7:1), orange/cyan accents colorblind-safe (not red/green only).
- Reduced-motion: visualizer updates at 10 Hz if OS reduced-motion enabled.

## Performance

- Editor timer 30 Hz (visualizer only); no per-sample GUI work.
- Spectral curve drawn from lock-free taps (copy 5 floats + 256-bin envelope, no alloc).
- When editor closed, `createEditor` not called → 0% UI CPU by construction.
