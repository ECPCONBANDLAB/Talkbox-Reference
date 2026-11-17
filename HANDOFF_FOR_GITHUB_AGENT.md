# Handoff for the GitHub-capable agent

**Project:** TalkBox Reference v1.0.0 — physically grounded, ultra-low-CPU
source-filter talkbox emulation (VST3 / AU / Standalone, C++20).

**Why this file exists:** everything except the GitHub publication is finished
and verified. The agent that built the project has no GitHub connector
(`list_connector_tools("github")` → `unsupported`), so the repository, the tag
and the release could not be created. This document hands the remaining work
over with every fact and command needed — no re-discovery required.

**Date:** 2026-11-17 · **Status:** ready to publish, tests green.

---

## 1. Get the code

Three interchangeable sources, all verified:

| # | Source | Details |
|---|---|---|
| A | **Source zip (stable link, no git history) — recommended** | `TalkBox-Reference-v1.0.0.zip`, 73 546 bytes, sha256 `944c4c2a03201e43df4064471ec2f8bd0dca2446cd0c4289383d8599d70209d2`<br>Drive id `1lf-RlwOH06sUecUgDMis07RtF5IJsJbc` — link in `artifacts/release_manifest.json` and in the handoff message |
| B | **git bundle (full history + tag)** | `TalkBox-Reference-v1.0.0.bundle` (~93 KB), uploaded to Drive under the name *"TalkBox Reference — TalkBox-Reference-v1.0.0.bundle"* (search the shared folder `0ALaaFQ0ZHDS9Uk9PVA`) and present at `artifacts/TalkBox-Reference-v1.0.0.bundle` in the workspace. Its sha256 changes whenever the tree changes, so verify with `git bundle verify` instead of a recorded hash. |
| C | Local checkout | `/home/user/talkbox-reference` — tag `v1.0.0`, 38+ tracked files, clean tree |

**Either A or B works.** B carries the tag and the 3-commit history, so the
release workflow fires as soon as the tag is pushed. With A you initialise the
repository yourself:

```bash
# path A only
unzip TalkBox-Reference-v1.0.0.zip && cd TalkBox-Reference-v1.0.0
chmod +x tools/*.sh packaging/*.sh "packaging/Uninstall TalkBox Reference.command"
git init -b main && git add -A
git commit -m "TalkBox Reference v1.0.0"
git tag -a v1.0.0 -m "TalkBox Reference v1.0.0 — first validated release (tests: PASS=40 FAIL=0)"
```

```bash
# From a machine with git + GitHub credentials
git clone TalkBox-Reference-v1.0.0.bundle TalkBox-Reference
cd TalkBox-Reference
git log --oneline          # 3 commits, tag v1.0.0 present
git tag -l                 # v1.0.0
```

> Note: if you receive the tree as a plain zip/folder instead of the bundle,
> the executable bits may be lost. Restore them before committing:
> `chmod +x tools/*.sh packaging/*.sh "packaging/Uninstall TalkBox Reference.command"`

---

## 2. What is DONE (verified, do not redo)

### 2.1 Code — complete and building

| Area | Files | State |
|---|---|---|
| DSP engine | `src/dsp/TbCore.h`, `TbSynthDriver.h`, `TbVocalTract.h`, `TbLpcMic.h`, `TbEngine.h` | ✅ dependency-free C++20, zero alloc/locks, FTZ/DAZ, NaN-safe |
| Plugin | `src/plugin/PluginProcessor.{h,cpp}`, `PluginEditor.{h,cpp}` | ✅ JUCE shell: APVTS, sidechain, L–R split, standalone mic |
| UI | `src/ui/VowelPad.h`, `src/ui/README.md` | ✅ vector GUI, vowel pad, 30 Hz visualizer |
| Tests | `tests/test_main.cpp` | ✅ 40 assertions, no external deps |
| Presets | `presets/factory_presets.json` | ✅ 4 factory presets |
| Packaging | `packaging/install.sh`, `uninstall.sh`, `TalkBoxReference.iss`, `Uninstall TalkBox Reference.command` | ✅ Win/macOS/Linux + portable mode |
| Build/CI | `CMakeLists.txt`, `.github/workflows/ci.yml` | ✅ |
| Docs | `docs/01` … `docs/07` | ✅ research & physics, DSP & accuracy, GUI/UX, perf & CPU, test & validation, user manual, install/uninstall |
| Meta | `README.md`, `CHANGELOG.md`, `LICENSE` (MIT), `.gitignore` | ✅ |

### 2.2 Verification already run (repeat after cloning to confirm)

```bash
sha256sum -c artifacts/SHA256SUMS.txt
# 15/15 OK — src/dsp/*.h (5), tests/test_main.cpp, presets, docs/01-07, CMakeLists.txt

g++ -std=c++20 -O2 -I src/dsp tests/test_main.cpp -o tests/run_tests && ./tests/run_tests
# Summary: PASS=40 FAIL=0 | formants=1 lpc=1 stability=1
# OVERALL: PASS
```

Test coverage: vowel formants (9 vowels + pure-tract + VTL scaling), live-mic
LPC tracking (whisper /i/, /ɑ/, /u/, bleed, sibilance), stability torture
(NaN/Inf/+120 dBFS/DC across 44.1–192 kHz × 16–2048 samples), CPU benchmarks.

**Numbers you will see will differ slightly from the archived ones** — CPU is
machine-dependent (documented ±30% in `docs/05`). The archived run (in
`artifacts/test_output.txt`) measured Eco 0.37%/0.72%, Balanced 0.53%/0.86%,
Studio 0.63%/0.99%; a re-run in a different sandbox gave 0.42/0.87, 0.58/0.93,
0.71/0.96. Both are inside budget (Eco <0.6/0.9%, Balanced <1.5%, Studio <3.5%).
The formant and stability results are deterministic and must match.

### 2.3 Already published to the cloud (nothing to do)

- **Google Drive** (shared folder `0ALaaFQ0ZHDS9Uk9PVA`): handoff report,
  test output, `factory_presets.json`, `SHA256SUMS.txt`.
- **Google Docs** (7 documents, one per `docs/0*.md`): links listed in
  `artifacts/release_manifest.json` under `cloud.docs`.
- **Added on top of the checksummed tree** (safe to keep): `.gitignore`,
  `tools/push_to_github.sh`, `tools/make_bundle.sh`,
  `.github/workflows/release.yml`, `RECOVERY_NOTES.md`, this file.

---

## 3. What is PENDING — your job

1. **Create the repository** `TalkBox-Reference` (owner: the user's account).
2. **Push** branch `main` and tag `v1.0.0`.
3. **Publish the release** `v1.0.0` with the assets and notes below.
4. **Report back** the three URLs (repo, tag, release).
5. **Optional follow-up:** update `artifacts/release_manifest.json` with the
   GitHub URL, then regenerate `artifacts/SHA256SUMS.txt` (it is itself inside
   the checksummed set — see the constraint in §5).

### Recommended commands

```bash
# 1. Push (repo missing → create it)
git remote add origin https://github.com/<owner>/TalkBox-Reference.git
git push -u origin main
git push origin v1.0.0

# …or with the GitHub CLI
gh repo create TalkBox-Reference --public --source=. --remote=origin --push
git push origin v1.0.0
```

Pushing the tag triggers `.github/workflows/release.yml`, which builds on
Linux/macOS/Windows, runs `ctest` (fails the release if any test is red) and
uploads `TalkBoxReference-*.zip` / `TalkBoxReference-*.tar.gz` to the release.
Let the workflow produce the binaries — do not upload locally built binaries.

### Repository metadata

- **Name:** `TalkBox-Reference`
- **Description:** `Physically grounded, ultra-low-CPU source-filter talkbox emulation (VST3/AU/Standalone).`
- **Topics:** `audio-plugin`, `talkbox`, `dsp`, `source-filter`, `waveguide`,
  `linear-prediction`, `juce`, `vst3`, `clap`, `cpp20`
- **License:** `mit` (matches `LICENSE`)
- **Default branch:** `main`
- **Homepage:** leave empty, or point at the Drive folder

### Release assets to attach (in addition to what the workflow uploads)

- `presets/factory_presets.json`
- `artifacts/test_output.txt` (rename to `test_output_v1.0.0.txt`)
- `artifacts/SHA256SUMS.txt`
- `artifacts/Final_Handoff_Report.md`
- Source zip/tar.gz of the tree at the tag

### Release notes (paste as-is)

```markdown
## TalkBox Reference v1.0.0

First validated release: a physically grounded, ultra-low-CPU source-filter
talkbox emulation.

**Signal chain**
Carrier (external / internal PolyBLEP synth) → DC blocker (18 Hz) →
compression driver (ADAA cubic soft-clip + 90 Hz HP / 6.5 kHz LP, 2×/4× OS) →
vinyl hose waveguide → vocal tract (normalised LAR lattice |k| ≤ 0.995 +
5× TPT resonators F1–F5 + nasal notch + lip closure) → lip radiation +
close-talk mic (4 models) → consonant bridge → dry/wet → out.

**Articulation:** vowel pad (IDW over 9 vowels), live mic (16 kHz LPC → LAR →
poles → TPT, < 6 ms), or MIDI.

**Measured (48 kHz / 128 samples, single core)**
- Formants at VTL = 1.0: F1 ≤ 2.3%, F2 ≤ 1.4%, F3 ≤ 1.6% over 9 vowels.
- Mic/whisper LPC: F1 1.1–15.4%, F2 0.1–5.5%; bleed < −60 dB; sibilance 0.50.
- CPU: Eco 0.37% pad / 0.72% mic; Balanced 0.53% / 0.86%; Studio 0.63% / 0.99%.
- Stability: 10 s torture, max 0.825, no blow-ups; bounded recovery across
  44.1–192 kHz × 16–2048 samples.
- Tests: 40/40 PASS (`g++ -std=c++20 -O2 -I src/dsp tests/test_main.cpp`).

**Formats:** VST3, AU (macOS), Standalone. No CLAP (JUCE 7.0.12 has no CLAP wrapper). AAX is not built (Avid licensing).

**Known limitations:** sustained vowels at VTL = 1.0 (adult-male anchor);
LPC validated on synthetic whispers; CPU figures are machine-dependent
(±30%); the JUCE shell needs JUCE 7+ (`-DTB_BUILD_JUCE=ON`) — the DSP tests
build without it.
```

---

## 4. Quick sanity checks before you push

```bash
git ls-files | wc -l                 # 38
git rev-parse v1.0.0                 # tag resolves
sha256sum -c artifacts/SHA256SUMS.txt   # 15/15 OK
g++ -std=c++20 -O2 -I src/dsp tests/test_main.cpp -o /tmp/t && /tmp/t | tail -1
# Summary: PASS=40 FAIL=0 | formants=1 lpc=1 stability=1
```

No `cmake` in the sandbox that built this, so the `ctest` path was **not**
executed locally — `CMakeLists.txt` defines `tb_tests` and `ci.yml` runs it on
Linux/macOS/Windows. Watch the first CI run.

---

## 5. Constraints — please respect these

1. **Do not modify the 15 files listed in `artifacts/SHA256SUMS.txt`** without
   regenerating that file afterwards. They are the byte-exact originals and the
   checksums are the integrity proof of the whole delivery.
2. **Do not merge a red CI.** The release workflow fails the release if any of
   the 40 tests fails.
3. **Do not upload binaries built outside CI** as release assets.
4. **Licence:** MIT is in place, but the maintainer has not yet confirmed the
   final licence choice for the public release — flag it, don't silently change.
5. **Trademark fallback:** if "TalkBox" is unavailable in a jurisdiction, the
   agreed fallback product name is **Vocal Hose Reference**.
6. **AAX is intentionally absent** (Avid licensing). Do not add it.

---

## 6. Background: what happened before you (context only)

The workspace that produced v1.0.0 was lost; only the published Google
Drive/Docs artifacts survived. The tree was rebuilt from those, then the
complete original tree was restored from a zip the user re-uploaded
(`1lf-RlwOH06sUecUgDMis07RtF5IJsJbc`), after which all 15 checksums verified
and the suite passed 40/40. Full record in `RECOVERY_NOTES.md`. The only
outstanding item has always been the GitHub publication, which is why you are
reading this.
