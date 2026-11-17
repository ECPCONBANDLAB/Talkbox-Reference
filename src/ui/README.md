# src/ui — Vector GUI components

- `VowelPad.h` — 2D Vowel & Mouth Visualizer (drag in Pad Mode, live-animated in Mic Mode). Implementation in `src/plugin/PluginEditor.cpp` (single JUCE target).
- Editor shell (`TalkBoxReferenceEditor`) in `src/plugin/PluginEditor.{h,cpp}`: header bar (Source/Carrier/Quality/Preset/Bypass), Simple page + Advanced tabs (Vocal Tract / Mic & Carrier / Hardware Chain / System).

Design: high-contrast, GPU-optional, DPI-scalable, no bitmaps. Visualizer timer 30 Hz; 0% UI CPU when editor closed. See `docs/03_GUI_and_UX_Specification.md`.
