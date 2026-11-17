#!/usr/bin/env bash
# TalkBox Reference 1.0.0 — uninstaller (macOS / Linux)
set -euo pipefail
exec "$(dirname "$0")/install.sh" --uninstall "$@"
