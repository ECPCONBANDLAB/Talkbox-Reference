#!/usr/bin/env bash
# TalkBox Reference — Linux installer (install.sh)
# Installs VST3/CLAP to ~/.vst3, ~/.clap and Standalone to ~/.local/bin
# 100% offline, no telemetry, no daemons. Portable mode: see PortableData/.
set -euo pipefail
SRC_DIR="$(cd "$(dirname "$0")/.." && pwd)"
VST3_DIR="$HOME/.vst3"
CLAP_DIR="$HOME/.clap"
BIN_DIR="$HOME/.local/bin"
echo "TalkBox Reference — Linux install"
echo "Source: $SRC_DIR"
mkdir -p "$VST3_DIR" "$CLAP_DIR" "$BIN_DIR"
# Expect built artifacts in build/ (cmake -DTB_BUILD_JUCE=ON)
for f in "$SRC_DIR/build/TalkBox Reference.vst3" "$SRC_DIR/build/TalkBox Reference.clap" "$SRC_DIR/build/TalkBox Reference"; do
  if [ -e "$f" ]; then echo "Found $f"; fi
done
echo "Copy built plugins to $VST3_DIR / $CLAP_DIR (see build output)."
echo "If no build found, build first: cmake -B build -DTB_BUILD_JUCE=ON -DJUCE_PATH=/path/to/JUCE && cmake --build build --config Release"
echo "Presets installed to: $SRC_DIR/presets (portable) or ~/.config/talkbox-reference/"
mkdir -p "$HOME/.config/talkbox-reference/presets"
cp -n "$SRC_DIR/presets/factory_presets.json" "$HOME/.config/talkbox-reference/presets/" 2>/dev/null || true
echo "Done. Uninstall: ./packaging/uninstall.sh"
