# A11 — package anmacgamer 2.2.1 beta 3 (for Antigravity, 2026-11-27)

Work only in `~/GameScrcpy-macos-antigravity` (branch `mac/antigravity`). Start with
`git merge --ff-only ccr-e32722c2-75w11u` (head `d010418` or newer). Project map and commands:
`.agents/rules/project.md`. No push, no tag, no GitHub release, no downloads, no `brew`: you build and check the DMG
and write the notes; Claude reviews, merges to `main`, tags `v2.2.1-beta3` and publishes after the user's go.

## 1. Release notes and changelog

- Rewrite `packaging/macos/RELEASE-NOTES-beta3.md` (English first, then Chinese, same content; short plain sentences,
  no marketing words, no emoji). The current file is from 2026-11-25 and misses most of the work. List only what is
  done and tested, from `git log v2.2.1-beta2..HEAD --oneline` (about 90 commits). Main points:
  - Mirroring without USB debugging: on the cable (AOA touch) plus the companion's picture and sound over Wi-Fi; one
    QR scan per computer; no share-screen dialog after the phone was connected once with USB debugging.
  - Companion 2.3: Android 17 look (one Settings-style page, wallpaper colours), the phone stays quiet while the Mac
    plays the sound, Mac-side volume independent of the phone, the previous keyboard comes back on disconnect.
  - Keep-on-top button; toolbar with Back/Home/Recents plus 5 chosen buttons.
  - Built-in Honkai: Star Rail profiles (phone and cloud version, PC layout, Tab locks the view).
  - Key hints use Mac key names; F keys need fn unless "Use F1, F2… as standard function keys" is on.
  - Wireless mirroring falls back to the adb tunnel when the phone cannot be reached directly.
  - Fixes: crash when a logic stopped, Disconnect on companion mirroring, quitting on SIGTERM, Keychain asked once per
    launch instead of many times.
  - The name: anmacgamer everywhere (app, data folder moves over once, companion).
  - Known issues: the first launch of a new version asks once for the Keychain password ("Always Allow");
    the companion must be installed once over USB debugging; screen capture shows black for apps that block it.
- Add a `2.2.1-beta3` section to `CHANGELOG.md` in the same style as `2.2.1-beta2`, with the link
  `https://github.com/wyb-070110/anmacgamer/releases/tag/v2.2.1-beta3`.

## 2. Build

```bash
cd ~/GameScrcpy-macos-antigravity/android && JAVA_HOME=/opt/homebrew/opt/openjdk@17 ANDROID_HOME=~/Library/Android/sdk \
  ./gradlew :server:testDebugUnitTest :server:assembleRelease :companion:assembleDebug --offline
cd .. && cmake --preset mac-app && cmake --build --preset mac-app && ctest --preset mac-app -j8   # all must pass
cmake --preset mac-release && cmake --build --preset mac-release                                   # universal
packaging/macos/package.sh beta3
```

The DMG lands at `build/package/anmacgamer-2.2.1-beta3-macos-universal.dmg` (you built beta 2 the same way).

## 3. Check the DMG

- `lipo -archs` on `Contents/MacOS/anmacgamer`, `gs-exthost`, `gs-scripthost`, `gs`: `x86_64 arm64`.
- `codesign --verify --deep --strict` passes; `otool -L` on the app and helpers shows nothing under `/opt/homebrew`
  or `~/Qt`.
- `Contents/Resources` has `adb`, `gs-server.apk`, `gs-companion.apk` (versionCode 10, 2.3.2 — check
  `gs-companion.json`) and `extensions/gamescrcpy.games/1.0.2`.
- Mount the DMG, copy the app to a temp folder, run it with `GS_DATA_DIR=<temp>` and
  `--smoke-test --demo-device tests/data/video/emulator_h264.media`: exit 0. Never touch the user's running app or data.

## 4. Report

At the end of `docs/handoff/antigravity.md`: commit hash, DMG path, size, SHA-256 (`shasum -a 256`), the checks and
their results, anything that failed. Commit the notes and changelog on `mac/antigravity`; mark it "ready to merge".
