#!/usr/bin/env bash
# TalkBox Reference — Linux clean uninstaller
# Prompts: Keep presets vs Remove all user data. Touches only own manifest.
set -euo pipefail
echo "TalkBox Reference — Clean Uninstall"
echo "Files that would be removed:"
echo "  ~/.vst3/TalkBox Reference.vst3"
echo "  ~/.clap/TalkBox Reference.clap"
echo "  ~/.local/bin/TalkBox Reference"
echo "  ~/.config/talkbox-reference/ (presets & settings)"
read -r -p "Remove all user data (100% clean)? [y/N] (N = keep presets): " ans
rm -rf "$HOME/.vst3/TalkBox Reference.vst3" "$HOME/.clap/TalkBox Reference.clap" "$HOME/.local/bin/TalkBox Reference"
if [[ "$ans" == "y" || "$ans" == "Y" ]]; then
  echo "Backing up presets to ~/talkbox-presets-backup.json ..."
  cp "$HOME/.config/talkbox-reference/presets/factory_presets.json" "$HOME/talkbox-presets-backup.json" 2>/dev/null || true
  rm -rf "$HOME/.config/talkbox-reference"
  echo "Removed all user data. Backup at ~/talkbox-presets-backup.json"
else
  echo "Kept presets & settings in ~/.config/talkbox-reference/"
fi
echo "Done. Nothing outside TalkBox Reference manifest was touched."
