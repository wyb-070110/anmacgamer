# A12 — rebuild the beta 3 DMG (for Antigravity, 2026-11-27)

Your A11 DMG is out of date: after it, the main line changed the in-app help (touch-box) and the companion (2.3.3,
versionCode 11, its Help link points at anmacgamer), and merged your A11 notes (`95c2a39`).

In `~/GameScrcpy-macos-antigravity` only:

1. `git merge --ff-only ccr-e32722c2-75w11u` (head `95c2a39` or newer).
2. Android build (`.agents/rules/project.md`, gradle line), then `cmake --build --preset mac-app`, `ctest --preset
   mac-app -j8` (176 pass), `cmake --build --preset mac-release`, `packaging/macos/package.sh beta3`.
3. The same DMG checks as A11; `gs-companion.json` must now say versionCode 11 / 2.3.3, and
   `Contents/Resources/help/*/touch-box.md` (or wherever the help is bundled) must not contain `NuoRan`.
4. Report at the end of `docs/handoff/antigravity.md`: path, size, SHA-256, checks. No push, no tag, no release.
