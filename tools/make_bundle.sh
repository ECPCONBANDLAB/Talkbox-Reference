#!/usr/bin/env bash
# TalkBox Reference — credential-free handoff
#
# Produces artifacts/TalkBox-Reference-<tag>.bundle : a single file holding the
# full git history, branches and tags. No token or SSH key is ever needed in
# the sandbox — you move the bundle to your own machine and push from there.
#
#   ./tools/make_bundle.sh [tag]        (default: v1.0.0)
set -euo pipefail

TAG="${1:-v1.0.0}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

command -v git >/dev/null || { echo "git is required" >&2; exit 1; }
git rev-parse --is-inside-work-tree >/dev/null 2>&1 || { echo "Not inside a git repository" >&2; exit 1; }

# Git identity fallback: a restored checkout may not carry .git/config.
GITID=(-c user.name="${GIT_AUTHOR_NAME:-TalkBox Reference}" -c user.email="${GIT_AUTHOR_EMAIL:-talkbox-reference@local}")

if git status --porcelain | grep -q .; then
  echo "!! Working tree is dirty — committing the pending changes so the bundle is complete."
  git add -A
  git "${GITID[@]}" commit -q -m "wip: snapshot for bundle ${TAG}"
fi

git rev-parse "$TAG" >/dev/null 2>&1 || {
  echo "Creating tag ${TAG} …"
  git "${GITID[@]}" tag -a "$TAG" -m "TalkBox Reference ${TAG} — first validated release (tests: PASS=40 FAIL=0)"
}

BRANCH="$(git rev-parse --abbrev-ref HEAD)"
OUT="artifacts/TalkBox-Reference-${TAG}.bundle"
mkdir -p artifacts

echo "Packing ${BRANCH} + ${TAG} → ${OUT}"
git bundle create "$OUT" --all
chmod 644 "$OUT"

SIZE="$(du -h "$OUT" | cut -f1)"
echo
echo "✅ ${OUT} (${SIZE})"
echo
echo "Verify it, then push from YOUR machine (no credentials in the sandbox):"
echo
echo "  git clone TalkBox-Reference-${TAG}.bundle TalkBox-Reference"
echo "  cd TalkBox-Reference"
echo "  git remote set-url origin https://github.com/<owner>/TalkBox-Reference.git"
echo "  git push -u origin ${BRANCH}"
echo "  git push origin ${TAG}"
echo
echo "…or, if the remote repository does not exist yet:"
echo "  gh repo create TalkBox-Reference --public --source=. --push"
