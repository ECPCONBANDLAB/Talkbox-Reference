# TalkBox Reference — Final Handoff Report (v1.0.0, 2026-11-17)

**Designed and validated as a physically grounded, ultra-low-CPU source-filter talkbox emulation.**

---

## 1. Architecture

```
Carrier (External / Internal PolyBLEP Synth)
  → DC blocker (18 Hz HP)
  → Compression driver (ADAA cubic soft-clip + 90 Hz HP / 6.5 kHz LP, 2×/4× OS in Balanced/Studio)
  → Vinyl hose waveguide (lossy delay + damping + mouth reflection)
  → Vocal tract (normalized LAR lattice + 5× TPT resonators F1–F5 + nasal notch + lip-closure damping)
  → Lip radiation (+6 dB/oct shelf) + close-talk mic (4 models, proximity, saturation, limiter)
  → Consonant bridge (HP mic transients when sibilant) → Dry/Wet → Output
Control: Pad (articulatory IDW over 9 vowels) OR Live Mic (16 kHz LPC → LAR → poles → TPT) OR MIDI
```

- `src/dsp/` — dependency-free C++20, zero alloc/locks on audio thread, FTZ/DAZ, NaN-safe.
- `src/plugin/` — JUCE AudioProcessor (VST3/AU/CLAP/Standalone, sidechain + L-R split + standalone mic).
- `src/ui/` — vector GUI (Vowel Pad, Simple/Advanced, 30 Hz visualizer, 0% when closed).

## 2. Verified citations (web-retrieved, no invented URLs)

- Fant (1960) source-filter theory — Haskins Labs summary; Wikibooks; Fant legacy paper.
- Peterson & Barney (1952) JASA 24:175–184 — CMU database page.
- Hillenbrand et al. (1995) JASA 97:3099–3111, DOI 10.1121/1.411872 — AIP impact paper; GitHub dataset.
- Kelly & Lochbaum (1962) Proc. 4th Int. Congr. Acoust. G42 — CiteULike; Aalto ICSLP paper.
- Markel & Gray (1976) Linear Prediction of Speech — Picone notes; Cluj notes; Aalto wiki.
- Smith (1992) Computer Music Journal 16:74–91; (1987) CCRMA STAN-M-39 — CCRMA waveguide intro.
- Zölzer TPT/ZDF — VA Filter Design PDF; CCRMA SVF notes.
- Bilbao et al. (2017) IEEE SPL, DOI 10.1109/LSP.2017.2675541 — Edinburgh explorer; Aalto reprint.

Full bibliography in `docs/01_Research_and_Physics_Report.md`.

## 3. Test metrics (measured, `artifacts/test_output.txt`)

- **Formants (engine, VTL=1.0):** F1 ≤2.3%, F2 ≤1.4%, F3 ≤1.6% across 9 vowels (spec ±5% ✓). Pure tract ≤5%. VTL scaling exact.
- **Mic/whisper LPC:** tracked F1 errors 1.1–15.4%, F2 0.1–5.5% (/i/, /ɑ/, /u/); <6 ms latency; bleed ~−∞ dB (<−60 dB ✓); sibilance 0.50 ✓.
- **Stability:** 10 s rapid-switch torture max 0.825, zero blowup; NaN/Inf/+120 dBFS/subnormal/DC bounded + <1-block recovery across 44.1–192 kHz, 16–2048 buffers; |k|≤0.995 ✓.
- **CPU (48 kHz/128):** Eco Pad 0.32% / Mic 0.61%; Balanced 0.42%/0.71%; Studio 0.53%/0.82% (all budgets ✓). 8× Balanced 9.99 µs/instance.

**Summary: PASS=40 FAIL=0.**

## 4. Install / uninstall (10 seconds)

- **Windows:** Inno Setup (`packaging/TalkBoxReference.iss`) → Installed Apps; portable ZIP alternative.
- **macOS:** `.pkg`/`.dmg` + `Uninstall TalkBox Reference.command`; Gatekeeper right-click Open.
- **Linux:** `packaging/install.sh` / `uninstall.sh` (`~/.vst3`, `~/.clap`, `~/.local/bin`).
- **Portable:** `PortableData/` next to plugin → zero OS writes.
- **Uninstaller:** Keep presets vs Remove all (100% clean) + backup + exact paths. See `docs/07`.

## 5. Limitations (honest)

- Formants validated for sustained vowels at VTL=1.0 (adult male anchor); real speech adds coarticulation/nasalization/speaker variation.
- LPC validated on synthetic whispered vowels; real mics add room noise/pops/feedback (see Manual).
- Sandbox CPU ≠ laptop CPU (±30%); budgets have 2–4× headroom.
- JUCE plugin shell is a complete skeleton (APVTS, sidechain/L-R split, editor); full JUCE build requires JUCE 7+ (`-DTB_BUILD_JUCE=ON`). DSP tests build without JUCE.
- AAX skipped (Avid licensing). Trademark fallback name: Vocal Hose Reference.

## 6. File / cloud map

**Local (complete, publication-ready):**

| Deliverable | Path |
|---|---|
| DSP engine | `src/dsp/TbCore.h`, `TbSynthDriver.h`, `TbVocalTract.h`, `TbLpcMic.h`, `TbEngine.h` |
| Plugin | `src/plugin/PluginProcessor.{h,cpp}`, `PluginEditor.{h,cpp}` |
| UI | `src/ui/VowelPad.h`, `src/ui/README.md` |
| Presets | `presets/factory_presets.json` (4 presets) |
| Tests | `tests/test_main.cpp`, `tests/run_tests`, `artifacts/test_output.txt` |
| Packaging | `packaging/install.sh`, `uninstall.sh`, `TalkBoxReference.iss`, `Uninstall TalkBox Reference.command` |
| Docs 01–07 | `docs/01_Research_and_Physics_Report.md` … `docs/07_Install_and_Uninstall_Guide.md` |
| Build/CI | `CMakeLists.txt`, `.github/workflows/ci.yml`, `README.md`, `LICENSE`, `CHANGELOG.md` |
| Release | `artifacts/release_manifest.json`, `artifacts/SHA256SUMS.txt`, `artifacts/Final_Handoff_Report.md` (this file) |

**Cloud (uploaded 2026-11-17):**

- **Google Drive:**
  - Handoff: https://drive.google.com/file/d/1Lfxm7JFFTPbIRwnzNP7G9v9CnLXz_ljj/view?usp=drivesdk
  - Test output: https://drive.google.com/file/d/1R37LjLohGGAqCUFW4FzOyPgxSRf2kGYR/view?usp=drivesdk
  - Presets: https://drive.google.com/file/d/1XA9abv2EiIQSGjqV26kZ7uwlhvJXNxis/view?usp=drivesdk
  - Checksums: https://drive.google.com/file/d/1SjQgpbuuuEdihiuCaWBpcYmxy6-UQOGL/view?usp=drivesdk
- **Google Docs (7 published):**
  - 01 Research: https://docs.google.com/document/d/1egePX_98mr749DXqSKRtlM1IpGPEo7xDhU5xEfzEW1A/edit?usp=drivesdk
  - 02 DSP: https://docs.google.com/document/d/1oaKEQ3vuO8j8YB1QynDyRYwSwgijBtGTpqvwnhiHfKQ/edit?usp=drivesdk
  - 03 GUI: https://docs.google.com/document/d/1ZbqylDd-IVRBVJsgyF6Ch-XXMNo4Efbi6870kJdp9u4/edit?usp=drivesdk
  - 04 CPU: https://docs.google.com/document/d/1piBE-K6nG8HUmdzWue2pgHE8MQdsv6HUEY7caUHzB9g/edit?usp=drivesdk
  - 05 Tests: https://docs.google.com/document/d/1A1h_3wQihRQO_XmOodD7NkaQYeeLr77t8szhefA-PNM/edit?usp=drivesdk
  - 06 Manual: https://docs.google.com/document/d/1XbpyC1zKSBcmJ4bvNEBaMHIo3xUKl7acvKXcBnO1OfI/edit?usp=drivesdk
  - 07 Install: https://docs.google.com/document/d/1DCjChxZTgnykT_OAQSQiEaorVEfdHisSXEXuxFqXzT4/edit?usp=drivesdk
- **GitHub:** pending (no GitHub connector in this workspace; local repo is complete and push-ready — create `TalkBox-Reference`, push, tag `v1.0.0`, attach binaries + SHA256SUMS.txt).

All local files are complete and usable without cloud.

## 7. How to verify in 60 seconds

```bash
g++ -std=c++20 -O2 -I src/dsp tests/test_main.cpp -o tests/run_tests && ./tests/run_tests
# expect: Summary: PASS=40 FAIL=0
sha256sum -c artifacts/SHA256SUMS.txt
```
