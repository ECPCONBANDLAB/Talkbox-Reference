# TalkBox Reference — 07 Install & Uninstall Guide

**Policy:** 100% offline, zero telemetry, zero background services/daemons, zero accounts/dongles. 10-second removal. Portable mode with zero OS writes.

---

## 1. Windows

### Installer (Inno Setup / WiX)

1. Download `TalkBoxReference-Setup-1.0.0.exe` (see `artifacts/release_manifest.json` for SHA-256).
2. Run installer → Next → Install. Registers cleanly in **Settings → Apps → Installed apps**.
3. Plugins installed to:
   - `C:\Program Files\Common Files\VST3\TalkBox Reference.vst3`
   - `C:\Program Files\Common Files\CLAP\TalkBox Reference.clap`
   - Standalone: `C:\Program Files\TalkBox Reference\TalkBox Reference.exe`
4. Restart your DAW to rescan. No reboot, no services, no registry bloat beyond standard plugin registration.

### Portable ZIP (zero-install)

1. Download `TalkBoxReference-Portable-1.0.0.zip`, unzip anywhere (e.g. `D:\Plugins\`).
2. Point your DAW's VST3/CLAP scan path at the folder. No installer, no admin rights.
3. Place a `PortableData/` folder next to the plugin/standalone to store presets locally (see §4).

### Uninstall (Windows)

- **Settings → Apps → TalkBox Reference → Uninstall** (clean uninstaller).
- Prompts: **Keep my presets & settings** vs **Remove all user data (100% clean)**. Offers one-click preset backup (`talkbox-presets-backup.json`) before removal. Shows exact paths deleted. Touches nothing outside its manifest.

## 2. macOS

### Package (.pkg / .dmg)

1. Download `.dmg`, open, drag to **Applications** (or run `.pkg`).
2. Supports user install (`~/Library/Audio/Plug-Ins/`) or system install (`/Library/Audio/Plug-Ins/`).
3. **Gatekeeper:** if blocked ("cannot be opened"), right-click → **Open** → **Open** (one-time). No developer account needed for local use; notarization required for public distribution (see §5).
4. Restart DAW (Logic, Ableton, etc.) to rescan. AU + VST3 + CLAP + Standalone included.

### Uninstall (macOS)

- Double-click **`Uninstall TalkBox Reference.command`** (in `packaging/` or Applications).
- Same Keep vs Remove prompt + backup. Removes only:
  - `~/Library/Audio/Plug-Ins/TalkBox Reference.component` (AU)
  - `~/Library/Audio/Plug-Ins/TalkBox Reference.vst3`
  - `~/Library/Audio/Plug-Ins/TalkBox Reference.clap`
  - `~/Library/Application Support/TalkBox Reference/` (if Remove all)

## 3. Linux

### Tarball (.tar.gz)

```bash
tar -xzf TalkBoxReference-1.0.0-linux.tar.gz
cd TalkBoxReference-1.0.0
./packaging/install.sh
# Installs to ~/.vst3, ~/.clap, ~/.local/bin
# Presets to ~/.config/talkbox-reference/presets/
```

### Standalone AppImage (optional)

```bash
chmod +x TalkBoxReference-1.0.0.AppImage
./TalkBoxReference-1.0.0.AppImage
```

### Uninstall (Linux)

```bash
./packaging/uninstall.sh
# Prompts Keep vs Remove, backs up presets, removes only own manifest
```

## 4. True Portable Mode

Place a `PortableData/` folder next to the plugin/standalone executable:

```
TalkBox Reference.vst3/
PortableData/
  presets/
  config.json
```

All presets and config store locally. Zero writes to registry, `AppData`, `Library`, or `~/.config`. Delete the folder to wipe. Ideal for USB/studio machines.

## 5. Signing / licensing notes (why no AAX)

- **AAX skipped:** Avid AAX requires Avid developer agreement + proprietary SDK + per-OS signing. Documented here per brief; VST3/AU/CLAP cover 99% of hosts.
- **Windows:** code-sign with standard cert (SmartScreen) for public release; unsigned local builds run fine.
- **macOS:** Apple Developer ID + notarization required for Gatekeeper-quiet distribution; local/unsigned builds work via right-click Open.
- **Linux:** no signing required.

## 6. Data folder locator

- Windows: `%APPDATA%\TalkBox Reference\` (or `PortableData\` if present)
- macOS: `~/Library/Application Support/TalkBox Reference/`
- Linux: `~/.config/talkbox-reference/`

System tab in plugin shows exact path + **Open Folder** button.

## 7. Verification

After install, run headless tests to confirm build:

```bash
./tests/run_tests  # expect: Summary: PASS=40 FAIL=0
```

SHA-256 checksums in `artifacts/SHA256SUMS.txt`. Release manifest in `artifacts/release_manifest.json`.
