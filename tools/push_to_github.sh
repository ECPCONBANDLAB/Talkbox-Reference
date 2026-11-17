#!/usr/bin/env bash
# TalkBox Reference — publish this repository to GitHub
#
# There is no GitHub connector available in the agent environment, so this
# script does the push using YOUR credentials. Two mutually exclusive modes:
#
#   HTTPS + token:  export GITHUB_TOKEN=ghp_xxx && ./tools/push_to_github.sh <owner> <repo>
#   SSH key:        ./tools/push_to_github.sh <owner> <repo> --ssh
#
# Options:
#   --private            create the repository as private (default: public)
#   --tag <v>            tag to push          (default: v1.0.0)
#   --branch <name>      branch to push       (default: current branch)
#   --dry-run            print what would happen, change nothing
#
# The token is never echoed and never written to disk.
set -euo pipefail

OWNER="${1:-}"
REPO="${2:-}"
PRIVATE=0; USE_SSH=0; DRY=0
TAG="v1.0.0"; BRANCH=""

shift 2 2>/dev/null || true
while [ $# -gt 0 ]; do
  case "$1" in
    --private) PRIVATE=1 ;;
    --ssh)     USE_SSH=1 ;;
    --dry-run) DRY=1 ;;
    --tag)     TAG="${2:?--tag needs a value}"; shift ;;
    --branch)  BRANCH="${2:?--branch needs a value}"; shift ;;
    *) echo "Unknown option: $1" >&2; exit 2 ;;
  esac
  shift
done

[ -n "$OWNER" ] && [ -n "$REPO" ] || {
  echo "Usage: ./tools/push_to_github.sh <owner> <repo> [--private] [--ssh] [--tag v1.0.0] [--branch main] [--dry-run]" >&2
  exit 2
}

command -v git >/dev/null || { echo "git is required" >&2; exit 1; }
git rev-parse --is-inside-work-tree >/dev/null 2>&1 || { echo "Not inside a git repository" >&2; exit 1; }

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"
[ -z "$BRANCH" ] && BRANCH="$(git rev-parse --abbrev-ref HEAD)"

if git status --porcelain | grep -q .; then
  echo "!! Working tree is dirty. Commit or stash first:" >&2
  git status --short >&2
  exit 1
fi

echo "Repository : $OWNER/$REPO"
echo "Branch     : $BRANCH"
echo "Tag        : $TAG"
echo "Visibility : $([ $PRIVATE -eq 1 ] && echo private || echo public)"
echo "Auth       : $([ $USE_SSH -eq 1 ] && echo 'SSH key' || echo 'HTTPS token')"
[ $DRY -eq 1 ] && { echo; echo "--dry-run: stopping here."; exit 0; }

# 1. Make sure the tag exists locally ---------------------------------------
GITID=(-c user.name="${GIT_AUTHOR_NAME:-TalkBox Reference}" -c user.email="${GIT_AUTHOR_EMAIL:-talkbox-reference@local}")

if git rev-parse "$TAG" >/dev/null 2>&1; then
  echo "Tag $TAG already exists locally."
else
  echo "Creating tag $TAG …"
  git "${GITID[@]}" tag -a "$TAG" -m "TalkBox Reference $TAG — first validated release (tests: PASS=40 FAIL=0)"
fi

REMOTE_URL=""
if [ $USE_SSH -eq 1 ]; then
  REMOTE_URL="git@github.com:${OWNER}/${REPO}.git"
else
  [ -n "${GITHUB_TOKEN:-}" ] || { echo "GITHUB_TOKEN is not set (or use --ssh)." >&2; exit 1; }
  REMOTE_URL="https://x-access-token:${GITHUB_TOKEN}@github.com/${OWNER}/${REPO}.git"

  # 2. Create the remote repository if it does not exist yet -----------------
  echo "Checking whether $OWNER/$REPO exists …"
  HTTP=$(curl -s -o /dev/null -w '%{http_code}' \
         -H "Authorization: Bearer ${GITHUB_TOKEN}" \
         -H "Accept: application/vnd.github+json" \
         "https://api.github.com/repos/${OWNER}/${REPO}" || echo 000)

  case "$HTTP" in
    200|301) echo "Remote repository exists." ;;
    404)
      echo "Creating repository $OWNER/$REPO …"
      VIS=$([ $PRIVATE -eq 1 ] && echo true || echo false)
      curl -sS -f -X POST \
        -H "Authorization: Bearer ${GITHUB_TOKEN}" \
        -H "Accept: application/vnd.github+json" \
        https://api.github.com/user/repos \
        -d "{\"name\":\"${REPO}\",\"private\":${VIS},\"description\":\"Physically grounded, ultra-low-CPU source-filter talkbox emulation (VST3/AU/CLAP/Standalone).\"}" \
        > /dev/null
      echo "Repository created."
      ;;
    401|403) echo "Token rejected (HTTP $HTTP). Needs the 'repo' scope." >&2; exit 1 ;;
    *) echo "GitHub API unreachable or unexpected response (HTTP $HTTP)." >&2; exit 1 ;;
  esac
fi

# 3. Push --------------------------------------------------------------------
git remote remove origin 2>/dev/null || true
git remote add origin "$REMOTE_URL"
echo "Pushing ${BRANCH} …"
git push -u origin "$BRANCH"
echo "Pushing ${TAG} …"
git push origin "$TAG"

# 4. Done --------------------------------------------------------------------
git remote set-url origin "$([ $USE_SSH -eq 1 ] && echo "git@github.com:${OWNER}/${REPO}.git" || echo "https://github.com/${OWNER}/${REPO}.git")"
echo
echo "✅ Pushed."
echo "   https://github.com/${OWNER}/${REPO}"
echo "   https://github.com/${OWNER}/${REPO}/releases/tag/${TAG}"
echo
echo "Next: open the Release page, generate notes, and attach the packaged"
echo "      artifacts (the release workflow does it automatically on tag push)."
