# anmacgamer — project rules for Antigravity

Read this first; it saves exploring. Full guides: `AGENTS.md`, `HANDOFF.md`, `TEAM.md`, `docs/architecture.md`.

## Where you work

- Your folder: `~/GameScrcpy-macos-antigravity`, branch `mac/antigravity` (a git worktree of the same repo).
- Main line: `ccr-e32722c2-75w11u` in `~/GameScrcpy-macos` (Claude). Start every task with
  `git merge --ff-only ccr-e32722c2-75w11u`. Never edit files in `~/GameScrcpy-macos`.
- No push, no release, no tags, no downloads, no `brew`. Ask the user (through Claude) before touching the phone.
- The task you are given is in `docs/handoff/antigravity-<task>.md`; report at the end of `docs/handoff/antigravity.md`.

## Map (open only what the task needs; read files in parts, not whole)

| Area | Mac (C++ / ObjC++, `src/`) | Phone (`android/`) |
|---|---|---|
| Devices, sessions, AOA, companion wiring | `app/DeviceManager.cpp` (large: search by function name) | — |
| Companion over Wi-Fi (GSCL / GSCM, pairing) | `app/CompanionLan.cpp`, `app/CompanionCipher.cpp`, `app/CompanionLink.cpp` | `companion/.../LanConnector.kt`, `Pairings.kt`, `LanCrypto.kt`, `CompanionService.kt` |
| Companion mirroring (picture + sound) | `session/src/CompanionMirror.cpp` | `companion/.../MirrorLink.kt`, `MirrorServer.kt`, `ScreenMirror.kt`, `SoundMirror.kt` |
| Session transports, handshake, reconnect | `session/src/` (`Session.cpp`, `AdbServerHost.cpp`, `StreamTransport.cpp`), `app/SessionController.cpp` | `server/` (Java, scrcpy-style server) |
| Keychain, signing, platform | `platform/src/mac/`, `platform/src/posix/` | — |
| Interface (QML) | `ui/`, `app/qml/` | `companion/.../Look.kt`, `Widgets.kt`, `HomePage.kt` |
| Protocol | `protocol/SPEC.md` (+ `protocol/tools/gen_vectors.py`) | `android/protocol/` |
| Tests | `tests/unit/<module>/`, `tests/integration/` (fake companion in the session tests) | `android/*/src/test/` |

Logs of the user's app: `~/Library/Application Support/anmacgamer/logs/gamescrcpy.log`.

## Build and check

```bash
cmake --build --preset mac-app            # after cmake --preset mac-app once
ctest --preset mac-app -j8                # 175 tests pass on the main line
python3 tools/check_source_rules.py       # no sleeps/fixed delays (R9) in new code, no dumpsys parsing
cd android && JAVA_HOME=/opt/homebrew/opt/openjdk@17 ANDROID_HOME=~/Library/Android/sdk \
  ./gradlew :server:testDebugUnitTest :server:assembleRelease :companion:assembleDebug --offline
```

- The mirror window runs without a phone: `build/mac-app/bin/anmacgamer.app/Contents/MacOS/anmacgamer --demo-device
  tests/data/video/emulator_h264.media` with `GS_DATA_DIR` set to a folder of your own (never the user's data).
- Quit the user's app only with `osascript -e 'tell application id "io.github.wyb070110.anmacgamer" to quit'`, and only
  when the user asked for a test.

## How to work fast

- Measure before changing: read the log lines around the failure first, then only the functions they name.
- Search (`grep -n`) for the function or message text instead of reading whole files.
- One task per conversation. Keep reports short: cause, change, how it was checked.
