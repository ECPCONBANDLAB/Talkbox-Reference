# 04 — Build and install

## Requirements

| Component | Minimum | Notes |
|---|---|---|
| C++ compiler | C++20 | GCC 12+, Clang 15+, MSVC 19.34+ |
| CMake | 3.22 | only for the plugin/CTest flow |
| JUCE | 7.0.9+ | **only** for the VST3/AU/CLAP/Standalone targets |
| OS | Windows 10 x64, macOS 11+, Linux x86_64 | |

The DSP core and the validation suite need **nothing but a compiler**.

## 1. Validate the DSP (no JUCE, ~5 s)

```bash
git clone https://github.com/ECPCONBANDLAB/Talkbox-Reference.git
cd Talkbox-Reference
g++ -std=c++20 -O2 -I src/dsp -I tests tests/test_main.cpp -o tests/run_tests
./tests/run_tests
# PASS=46 FAIL=0
```

CMake equivalent:

```bash
cmake -S . -B build -DTB_BUILD_TESTS=ON -DTB_BUILD_JUCE=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## 2. Build the plugin (JUCE 7)

Point CMake at a JUCE checkout, or let it fetch JUCE 7.0.12:

```bash
cmake -S . -B build -DTB_BUILD_JUCE=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --config Release
```

Offline / pinned JUCE:

```bash
cmake -S . -B build -DTB_BUILD_JUCE=ON -DTB_JUCE_PATH=/path/to/JUCE
```

Artefacts land in `build/TalkBoxReference_artefacts/Release/`:

```
Standalone/TalkBox Reference(.exe|.app)
VST3/TalkBox Reference.vst3
AU/TalkBox Reference.component          (macOS only)
CLAP/TalkBox Reference.clap
```

Enable the plugin formats you need by editing the `FORMATS` list in
`CMakeLists.txt` (e.g. `FORMATS VST3 Standalone` to skip AU/CLAP).

### Platform notes

* **Windows** — Visual Studio 2022 ("Desktop development with C++"). ASIO is
  optional: add `juce::juce_audio_devices` with
  `JUCE_ASIO=1` and the Steinberg SDK.
* **macOS** — Xcode 14+. For notarised distribution, sign with a Developer ID,
  then `xcrun notarytool submit` and `xcrun stapler staple`.
* **Linux** — `sudo apt install build-essential cmake libasound2-dev
  libfreetype-dev libfontconfig1-dev libx11-dev libxext-dev libxinerama-dev
  libxrandr-dev libxcursor-dev libgl1-mesa-dev libwebkit2gtk-4.1-dev`.

## 3. Install

### Windows

```powershell
iscc packaging\installer.iss
# -> Output\TalkBoxReference-1.0.0-win64.exe
```

The installer offers Standalone + VST3 + CLAP, per-user or per-machine
(`PrivilegesRequired=admin`), desktop shortcut and uninstall entry. Expected
paths: `%COMMONPROGRAMFILES%\VST3\TalkBox Reference.vst3`,
`%COMMONPROGRAMFILES%\CLAP\TalkBox Reference.clap`.

### macOS / Linux

```bash
./packaging/install.sh --user      # ~/Library/Audio/Plug-Ins/... or ~/.vst3
./packaging/install.sh --system    # /Library/... or /usr/lib/... (sudo)
./packaging/uninstall.sh           # remove every installed artefact
```

### Portable

Copy `packaging/portable/` next to the built binaries; see its `README.txt`.

## 4. Continuous integration

`.github/workflows/ci.yml` runs three jobs:

1. **dsp-tests** — Ubuntu + macOS + Windows, builds the suite with `-O2` and
   runs it under CTest.
2. **sanitizers** — Ubuntu with `-fsanitize=address,undefined` on the suite.
3. **plugin-build** — Ubuntu with JUCE 7.0.12 fetched by CMake, builds the VST3
   and Standalone targets to prove the shell compiles.

## 5. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `JUCE not found` | Pass `-DTB_JUCE_PATH=` or allow FetchContent to reach GitHub |
| DAW does not list the AU | `killall -9 AudioComponentRegistrar`, then rescan |
| Plugin validated but silent | The carrier bus must receive audio, or set *Carrier* to an internal oscillator |
| High CPU in a host | Lower *Oversampling* to 1x and pick the *Eco Pad* preset |
| Latency compensation looks off | Use the host's PDC; the plugin reports 36 samples at any rate |
