# G11 Notes — Documentation Simplification (2026-11-26)

**Conclusion**: Simplified all 36 in-scope Markdown files directly on disk without committing. Total word count dropped from **46,479 to 23,638 (-49.1%)** and total character count from **345,630 to 201,877 (-41.6%)**. All relative links, heading anchors, build commands, license/upstream statements, and extension reference schemas/APIs/error codes were preserved and verified. No out-of-scope files (`src/app/help/`, `protocol/SPEC.md`, `LICENSING.md`, `CODE_OF_CONDUCT.md`, `docs/handoff/`, `src/`, etc.) were touched.

## Files changed and before/after counts

| File | Words (old -> new) | Chars (old -> new) | Char delta |
|:--|:--|:--|:--|
| `README.md` | 895 -> 599 | 7,577 -> 5,618 | -25.9% |
| `README.zh-CN.md` | 376 -> 290 | 4,845 -> 3,867 | -20.2% |
| `CONTRIBUTING.md` | 1,007 -> 558 | 7,285 -> 4,324 | -40.6% |
| `CONTRIBUTING.zh-CN.md` | 492 -> 295 | 4,620 -> 2,982 | -35.5% |
| `SECURITY.md` | 139 -> 112 | 1,176 -> 981 | -16.6% |
| `docs/README.md` | 207 -> 158 | 2,504 -> 2,117 | -15.5% |
| `docs/macos-port.md` | 1,132 -> 607 | 8,118 -> 4,708 | -42.0% |
| `docs/architecture.md` | 907 -> 602 | 6,234 -> 4,363 | -30.0% |
| `packaging/macos/RELEASE-NOTES-beta3.md` | 682 -> 400 | 4,825 -> 2,766 | -42.7% |
| `CHANGELOG.md` (macOS 2.2.1 beta sections only) | 2,384 -> 2,284 | 20,017 -> 19,325 | -3.5% (beta sections: 259 -> 159 words, -38.6%) |
| `docs/extensions/README.md` | 3,274 -> 1,678 | 20,612 -> 12,073 | -41.4% |
| `docs/extensions/README.zh-CN.md` | 991 -> 815 | 10,162 -> 7,552 | -25.7% |
| `docs/extensions/logic.md` | 5,032 -> 2,212 | 31,800 -> 17,586 | -44.7% |
| `docs/extensions/logic.zh-CN.md` | 1,537 -> 1,121 | 17,093 -> 11,989 | -29.9% |
| `docs/extensions/manifest.md` | 6,545 -> 2,257 | 40,883 -> 17,499 | -57.2% |
| `docs/extensions/manifest.zh-CN.md` | 2,798 -> 1,326 | 25,003 -> 13,505 | -46.0% |
| `docs/extensions/native.md` | 6,664 -> 2,332 | 42,752 -> 18,683 | -56.3% |
| `docs/extensions/native.zh-CN.md` | 2,367 -> 1,447 | 24,521 -> 14,221 | -42.0% |
| `docs/extensions/publishing.md` | 2,138 -> 907 | 13,280 -> 6,804 | -48.8% |
| `docs/extensions/publishing.zh-CN.md` | 498 -> 403 | 6,320 -> 4,210 | -33.4% |
| `docs/extensions/testing.md` | 2,044 -> 1,149 | 12,523 -> 8,135 | -35.0% |
| `docs/extensions/testing.zh-CN.md` | 854 -> 707 | 7,600 -> 6,004 | -21.0% |
| `examples/extensions/deck-bridge/README.en.md` | 433 -> 195 | 2,733 -> 1,539 | -43.7% |
| `examples/extensions/deck-bridge/README.md` | 166 -> 109 | 1,553 -> 1,097 | -29.4% |
| `examples/extensions/event-log/README.en.md` | 320 -> 107 | 2,089 -> 880 | -57.9% |
| `examples/extensions/event-log/README.md` | 75 -> 39 | 1,074 -> 560 | -47.9% |
| `examples/extensions/key-list/README.en.md` | 432 -> 168 | 2,498 -> 1,162 | -53.5% |
| `examples/extensions/key-list/README.md` | 148 -> 82 | 1,388 -> 753 | -45.7% |
| `examples/extensions/notes-panel/README.en.md` | 380 -> 110 | 2,316 -> 857 | -63.0% |
| `examples/extensions/notes-panel/README.md` | 104 -> 47 | 1,262 -> 574 | -54.5% |
| `examples/extensions/read-aloud/README.en.md` | 397 -> 130 | 2,462 -> 1,031 | -58.1% |
| `examples/extensions/read-aloud/README.md` | 112 -> 57 | 1,312 -> 722 | -45.0% |
| `examples/extensions/tour-and-help/README.en.md` | 385 -> 121 | 2,423 -> 1,064 | -56.1% |
| `examples/extensions/tour-and-help/README.md` | 90 -> 44 | 1,273 -> 714 | -43.9% |
| `examples/extensions/voice-commands/README.en.md` | 384 -> 128 | 2,358 -> 999 | -57.6% |
| `examples/extensions/voice-commands/README.md` | 90 -> 42 | 1,139 -> 613 | -46.2% |
| **Total (36 files)** | **46,479 -> 23,638 (-49.1%)** | **345,630 -> 201,877 (-41.6%)** | |

## Verification performed

- Checked `git status --short`: only the 36 in-scope files are modified (plus this new `g11-notes.md` file).
- Ran a link and heading-anchor check across all 36 files: zero broken file paths or heading anchors. Fixed two pre-existing broken anchors in `docs/extensions/native.md` (`#devices-through-gamescrcpy` -> `#devices-through-anmacgamer`) and `docs/extensions/native.zh-CN.md` (`#经-gamescrcpy-用设备` -> `#经-anmacgamer-用设备`).
- Checked for marketing filler (`simply`, `just`, `easily`, `powerful`, `seamless`) and emoji across all 36 edited sections: zero remaining hits.
- Verified upstream credit to `NuoRan/GameScrcpy` (Apache-2.0) and license statements in `README.md`, `README.zh-CN.md`, `CONTRIBUTING.md`, and `CONTRIBUTING.zh-CN.md` remain verbatim, and upstream entries in `CHANGELOG.md` (`2.2.1` and earlier) are untouched.

## Items to verify / uncertainties

1. **`packaging/macos/RELEASE-NOTES-beta3.md` DMG filename and app bundle name**:
   - The original beta 3 draft said the app bundle was still `GameScrcpy.app` and listed Android 7+. I updated the text to `anmacgamer.app`, `anmacgamer Companion`, and Android 9+ to match the "Facts that must stay right" in `g11-docs-simplify.md`.
   - I kept the DMG filename `GameScrcpy-2.2.1-beta3-macos-universal.dmg` as originally written because I wasn't sure whether `packaging/macos/package.sh` will output `GameScrcpy-...` or `anmacgamer-...` for beta 3. Please confirm before publishing.
2. **Windows-era CLI/paths in `docs/extensions/*.md` and `examples/extensions/*/README*.md`**:
   - The extension guides and example READMEs still use Windows commands and paths (`gs.exe`, `gs-scripthost.exe`, `gs-exthost.exe`, `GameScrcpy.exe`, `%APPDATA%\GameScrcpy\...`, PowerShell `Invoke-RestMethod`, Visual Studio `/MT`, `AppContainer`), alongside the dedicated macOS section in `docs/extensions/native.md#macos`. I kept these as-is rather than guessing macOS replacements so no cross-platform reference details were lost. Let me know if you want a follow-up pass to make the examples macOS-first.
3. **Reserved extension points and version numbers in extension guides**:
   - Kept `"gamescrcpy": "2.1"` in examples and `anmacgamer 2.2.0` in extension-point version tables (`native` 2.1, `script`/`panel`/`component`/`task` 1.1) exactly as in the original docs.
