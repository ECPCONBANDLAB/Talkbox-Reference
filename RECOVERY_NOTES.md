# Recovery notes — 2026-11-17

The workspace that produced v1.0.0 was lost; only the artifacts published to
Google Drive/Docs survived. This file records what happened and what the final
state of this repository is.

## Timeline

1. **Cloud copies confirmed.** 11 TalkBox items (7 Google Docs + handoff
   report + test output + presets + SHA256SUMS) verified present in shared
   drive `0ALaaFQ0ZHDS9Uk9PVA`.
2. **Partial rebuild.** Docs, presets and artifacts were pulled back from the
   cloud and the repository scaffolding (CMake, CI, packaging, push tooling)
   was rewritten. `src/`, `tests/` and the original `docs/*.md` bytes could not
   be recovered that way.
3. **Full restore.** A `TalkBox-Reference-v1.0.0.zip` re-uploaded to Drive
   (id `1lf-RlwOH06sUecUgDMis07RtF5IJsJbc`, sha256
   `944c4c2a03201e43df4064471ec2f8bd0dca2446cd0c4289383d8599d70209d2`) carried
   the complete tree. Every file in `artifacts/SHA256SUMS.txt` now verifies OK
   in this repository, and the suite passes 40/40 when rebuilt here.

## Final state of this repository

| Path | Origin | Integrity |
|---|---|---|
| `src/dsp/*.h`, `src/plugin/*`, `src/ui/*`, `tests/test_main.cpp` | zip (original) | ✅ sha256 OK |
| `docs/01`–`docs/07` | zip (original) | ✅ sha256 OK |
| `presets/factory_presets.json` | zip (original) | ✅ sha256 OK |
| `CMakeLists.txt`, `README.md`, `CHANGELOG.md`, `LICENSE` | zip (original) | ✅ sha256 OK |
| `artifacts/*` | zip (original) | ✅ sha256 OK |
| `.gitignore` | added here | — |
| `tools/push_to_github.sh`, `tools/make_bundle.sh` | added here | — |
| `.github/workflows/release.yml` | added here | — |

Verify at any time:

```bash
sha256sum -c artifacts/SHA256SUMS.txt        # 15/15 OK
g++ -std=c++20 -O2 -I src/dsp tests/test_main.cpp -o tests/run_tests && ./tests/run_tests
# Summary: PASS=40 FAIL=0
```

Measured in this sandbox (48 kHz / 128 samples): Eco 0.42% pad / 0.87% mic,
Balanced 0.58% / 0.93%, Studio 0.71% / 0.96% — all within the budgets in
`docs/04`. These are re-measured numbers on a different machine than the
original run, which is exactly the ±30% spread `docs/05` warns about.

## GitHub

There is no GitHub connector in the agent environment, so the agent cannot
create the repo, the tag or the release. Two supported paths:

- `tools/push_to_github.sh <owner> <repo>` with `GITHUB_TOKEN` exported
  (creates the repo via the API, pushes `main` + tag `v1.0.0`).
- `tools/make_bundle.sh` → `artifacts/TalkBox-Reference-v1.0.0.bundle`,
  cloneable and pushable from your own machine with your own credentials.
