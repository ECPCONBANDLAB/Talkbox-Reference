#!/usr/bin/env bash
# TalkBox Reference 1.0.0 — macOS / Linux installer
# Usage: ./install.sh [--user|--system] [--uninstall]
set -euo pipefail

MODE="user"
ACTION="install"
for arg in "$@"; do
  case "$arg" in
    --system) MODE="system" ;;
    --user) MODE="user" ;;
    --uninstall) ACTION="uninstall" ;;
    -h|--help) sed -n '2,8p' "$0"; exit 0 ;;
    *) echo "unknown option: $arg" >&2; exit 2 ;;
  esac
done

OS="$(uname -s)"
if [[ "$OS" == "Darwin" ]]; then
  if [[ "$MODE" == "system" ]]; then
    VST3_DIR="/Library/Audio/Plug-Ins/VST3"; AU_DIR="/Library/Audio/Plug-Ins/Components"; CLAP_DIR="/Library/Audio/Plug-Ins/CLAP"; APP_DIR="/Applications"
  else
    VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"; AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"; CLAP_DIR="$HOME/Library/Audio/Plug-Ins/CLAP"; APP_DIR="$HOME/Applications"
  fi
else
  if [[ "$MODE" == "system" ]]; then
    VST3_DIR="/usr/lib/vst3"; CLAP_DIR="/usr/lib/clap"; APP_DIR="/opt"
  else
    VST3_DIR="$HOME/.vst3"; CLAP_DIR="$HOME/.clap"; APP_DIR="$HOME/.local/opt"
  fi
fi

ART="build/TalkBoxReference_artefacts/Release"

uninstall() {
  rm -rf "$VST3_DIR/TalkBox Reference.vst3" "$CLAP_DIR/TalkBox Reference.clap" "$APP_DIR/TalkBox Reference.app"
  if [[ -f "$AU_DIR/TalkBox Reference.component/Contents/Info.plist" ]]; then rm -rf "$AU_DIR/TalkBox Reference.component"; fi
  echo "TalkBox Reference removed from $MODE locations."
}

install_plugins() {
  mkdir -p "$VST3_DIR" "$CLAP_DIR"
  if [[ -d "$ART/VST3/TalkBox Reference.vst3" ]]; then
    rm -rf "$VST3_DIR/TalkBox Reference.vst3"
    cp -R "$ART/VST3/TalkBox Reference.vst3" "$VST3_DIR/"
    echo "  VST3 -> $VST3_DIR"
  fi
  if [[ -f "$ART/CLAP/TalkBox Reference.clap" ]]; then
    cp "$ART/CLAP/TalkBox Reference.clap" "$CLAP_DIR/"
    echo "  CLAP -> $CLAP_DIR"
  fi
  if [[ "$OS" == "Darwin" && -d "$ART/AU/TalkBox Reference.component" ]]; then
    mkdir -p "$AU_DIR"; rm -rf "$AU_DIR/TalkBox Reference.component"
    cp -R "$ART/AU/TalkBox Reference.component" "$AU_DIR/"
    echo "  AU   -> $AU_DIR"
  fi
  if [[ -d "$ART/Standalone/TalkBox Reference.app" ]]; then
    mkdir -p "$APP_DIR"; rm -rf "$APP_DIR/TalkBox Reference.app"
    cp -R "$ART/Standalone/TalkBox Reference.app" "$APP_DIR/"
    echo "  APP  -> $APP_DIR"
  fi
}

if [[ "$ACTION" == "uninstall" ]]; then uninstall; exit 0; fi
install_plugins
if [[ "$OS" == "Darwin" ]]; then
  echo "If a DAW does not see the AU yet, run: killall -9 AudioComponentRegistrar"
fi
echo "Done ($MODE install)."
