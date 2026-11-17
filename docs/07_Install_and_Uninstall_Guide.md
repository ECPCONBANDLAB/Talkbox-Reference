# TalkBox Reference — 07 Install & Uninstall Guide

**Policy:** 100% offline, zero telemetry, zero background services/daemons, zero accounts/dongles. 10-second removal. Portable mode with zero OS writes.

---

## 0. Read this first — v1.0.0 ships no installers

**There is no `TalkBoxReference-Setup-1.0.0.exe`, no portable ZIP, no `.dmg`, no `.pkg`, no AppImage and no prebuilt plugin of any kind in the v1.0.0 release.**

The v1.0.0 release contains source code, documentation, presets and the test report only. Building the plugin is a source build, described in §1 below.

Two reasons this is deliberate:

1. **Licensing.** `src/plugin/` links JUCE 7.0.12, which is dual-licensed **GPLv3 or paid commercial**. Publishing binaries therefore requires publishing them under GPLv3, or buying a JUCE licence. The repository is MIT, so shipping an MIT-labelled binary that statically links JUCE would misstate the terms. See `LICENSE-NOTES.md`.
2. **No signing.** There is no Authenticode certificate and no Apple Developer ID / notarisation in place, so any installer would be blocked by SmartScreen on Windows and Gatekeeper on macOS.

Earlier revisions of this document described installers as if they existed. They never did. That was an error and it is corrected here.

---

## 1. Build from source (the supported path)

### Prerequisites

- CMake 3.22 or newer
- A C++20 compiler (GCC 12+, Clang 15+, MSVC 2022)
- JUCE **7.0.12** — either cloned locally or fetched automatically
- Linux only: `libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev libasound2-dev`

### Headless DSP tests (no JUCE needed, ~3 s)

```bash
g++ -std=c++20 -O2 -Wall -Wextra -I src/dsp tests/test_main.cpp -o tests/run_tests
./tests/run_tests        # expect: Summary: PASS=40 FAIL=0
```

### The plugin (VST3 + Standalone; AU on macOS)

```bash
# JUCE 7.0.12 is pinned; use the same version or the wrappers change
git clone --depth 1 --branch 7.0.12 https://github.com/juce-framework/JUCE third_party/JUCE

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DTB_BUILD_JUCE=ON -DJUCE_PATH=third_party/JUCE
cmake --build build --config Release
```

`TB_BUILD_JUCE` defaults to **OFF**, so a plain `cmake -B build` builds only the DSP tests.

### Where the artifacts land

JUCE writes each format into its own folder under `build/TalkBoxReference_artefacts/Release/`:

| Platform | Path written by the build |
|---|---|
| Windows | `TalkBoxReference_artefacts\Release\VST3\TalkBox Reference.vst3\`<br>`TalkBoxReference_artefacts\Release\Standalone\TalkBox Reference.exe` |
| macOS | `TalkBoxReference_artefacts/Release/VST3/TalkBox Reference.vst3`<br>`TalkBoxReference_artefacts/Release/AU/TalkBox Reference.component`<br>`TalkBoxReference_artefacts/Release/Standalone/TalkBox Reference.app` |
| Linux | `TalkBoxReference_artefacts/Release/VST3/TalkBox Reference.vst3`<br>`TalkBoxReference_artefacts/Release/Standalone/TalkBox Reference` |

Copy the `.vst3` bundle into your system VST3 folder and the standalone wherever you keep apps. Restart the DAW to rescan.

### Formats actually supported

**VST3, AU (macOS only) and Standalone.** There is **no CLAP build**: JUCE 7.0.12 has no CLAP wrapper — there is no `juce_audio_plugin_client/CLAP` directory and no CLAP entry in `_juce_get_platform_plugin_kinds`, so a `CLAP` token in `FORMATS` is silently ignored. `CMakeLists.txt` documents this. AAX is skipped for Avid SDK licensing reasons (see §5).

## 2. Windows (manual, after a source build)

There is no installer. To place a build system-wide, copy the `.vst3` bundle to:

```
C:\Program Files\Common Files\VST3\
```

If you want an installer anyway, `packaging/TalkBoxReference.iss` is an Inno Setup script, but note that its `[Files]` section expects a `build\` layout — adjust the `Source:` paths to the `_artefacts` paths in the table above before compiling it. It is **not** built or tested by CI.

### Manual uninstall (Windows)

Delete the `.vst3` bundle and the standalone folder you copied. Nothing is registered in the registry or in services, so there is nothing else to remove. Presets live in `%APPDATA%\TalkBox Reference\` (see §6).

## 3. macOS (manual, after a source build)

Copy the built bundles to a plug-in folder:

- Per-user: `~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/Components/`
- System-wide: `/Library/Audio/Plug-Ins/VST3/` and `/Library/Audio/Plug-Ins/Components/`

**Gatekeeper:** because the build is unsigned, macOS may refuse to open the Standalone the first time. Right-click → **Open** → **Open**. Notarisation is required only for distributing builds to other people (§5).

### Manual uninstall (macOS)

Delete the `.vst3` and `.component` bundles you copied. `packaging/Uninstall TalkBox Reference.command` is provided, but verify its paths match where you installed before running it.

## 4. Linux (manual, after a source build)

```bash
mkdir -p ~/.vst3
cp -r build/TalkBoxReference_artefacts/Release/VST3/"TalkBox Reference.vst3" ~/.vst3/
build/TalkBoxReference_artefacts/Release/Standalone/"TalkBox Reference"
```

`packaging/install.sh` exists and installs to `~/.vst3`, `~/.clap` and `~/.local/bin`. The `.clap` step will have nothing to copy, because no CLAP build is produced.

## 5. True Portable Mode

Place a `PortableData/` folder next to the plugin/standalone executable:

```
TalkBox Reference.vst3/
PortableData/
```

Presets and settings are then read and written inside `PortableData/` instead of the OS locations in §6, so nothing is written outside the folder. Deleting the folder removes every trace.

## 6. Signing / licensing notes (why no AAX)

- **AAX skipped:** Avid AAX requires an Avid developer agreement, the proprietary SDK and per-OS signing. VST3/AU cover the large majority of hosts.
- **Windows:** code-sign with a standard certificate to avoid SmartScreen warnings; unsigned local builds run fine.
- **macOS:** an Apple Developer ID plus notarisation is required for Gatekeeper-quiet *distribution*. Local unsigned builds work via right-click Open.
- **Linux:** no signing required.
- **JUCE:** see `LICENSE-NOTES.md`. Source is MIT; binaries linking JUCE are GPLv3-or-commercial.

## 7. Data folder locator

- Windows: `%APPDATA%\TalkBox Reference\` (or `PortableData\` if present)
- macOS: `~/Library/Application Support/TalkBox Reference/`
- Linux: `~/.config/talkbox-reference/`

The System tab in the plugin shows the exact path.

## 8. Verification

After a build, confirm the DSP engine is intact:

```bash
./tests/run_tests   # expect: Summary: PASS=40 FAIL=0
```

SHA-256 checksums of the tracked source, docs and presets are in `artifacts/SHA256SUMS.txt`; verify with `sha256sum -c artifacts/SHA256SUMS.txt`. Release metadata is in `artifacts/release_manifest.json`.
