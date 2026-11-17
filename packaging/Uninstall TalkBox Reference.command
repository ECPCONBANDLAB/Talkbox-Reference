#!/bin/bash
# TalkBox Reference — macOS double-click uninstaller
# Removes ~/Library/Audio/Plug-Ins/TalkBox Reference.component/.vst3/.clap
# Prompts Keep presets vs Remove all. Gatekeeper: right-click > Open if blocked.
echo "TalkBox Reference — Clean Uninstall (macOS)"
echo "Files that would be removed:"
echo "  ~/Library/Audio/Plug-Ins/TalkBox Reference.component"
echo "  ~/Library/Audio/Plug-Ins/TalkBox Reference.vst3"
echo "  ~/Library/Audio/Plug-Ins/TalkBox Reference.clap"
echo "  ~/Library/Application Support/TalkBox Reference/ (presets & settings)"
read -r -p "Remove all user data (100% clean)? [y/N] (N = keep presets): " ans
rm -rf "$HOME/Library/Audio/Plug-Ins/TalkBox Reference.component" \
       "$HOME/Library/Audio/Plug-Ins/TalkBox Reference.vst3" \
       "$HOME/Library/Audio/Plug-Ins/TalkBox Reference.clap"
if [[ "$ans" == "y" || "$ans" == "Y" ]]; then
  cp "$HOME/Library/Application Support/TalkBox Reference/presets/factory_presets.json" "$HOME/talkbox-presets-backup.json" 2>/dev/null || true
  rm -rf "$HOME/Library/Application Support/TalkBox Reference"
  echo "Removed all user data. Backup at ~/talkbox-presets-backup.json"
else
  echo "Kept presets & settings."
fi
echo "Done. Restart your DAW to rescan plugins."
