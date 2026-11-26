# A10 — no-adb mirroring: pairing survives, mirror end disconnects (for Antigravity, 2026-11-26)

Work in `~/GameScrcpy-macos-antigravity` on `mac/antigravity`. First fast-forward to the main line:
`git merge --ff-only ccr-e32722c2-75w11u` (head `1042dba` or newer). No push, no release, stay in your folder.
Write your report at the end of `docs/handoff/antigravity.md` as before.

## The setup

Phone with USB debugging **off**, on the cable (AOA touch), companion app 2.3.0 paired over Wi-Fi by QR code.
The Mac shows the no-picture AOA control window; when the companion's mirror link (`GSCM`, SPEC 11.10) arrives the
window gives way to a real session: picture and sound from the companion over Wi-Fi, touch over AOA
(`DeviceManager::mirrorArrived` → `switchToCompanionPicture`). Log: `~/Library/Application Support/anmacgamer/logs/gamescrcpy.log`.

Code: Mac `src/app/CompanionLan.cpp` (listener, GSCL/GSCM handshakes, `m_expected`), `src/app/DeviceManager.cpp`
(`syncCompanionLan`, `syncLanService`, `lanArrived`, `mirrorArrived`, `switchToCompanionPicture`, the
`CompanionEvent::Connected` handler that saves the pairing to `devices.json` via `pairingText`),
`src/session/src/CompanionMirror.cpp`; companion `android/companion/.../LanConnector.kt` (`pair`, `reconnect`,
`attempt`), `CompanionService.kt` (`mirrorOver`), `MirrorLink.kt`, `MirrorServer.kt`.

## Bug 1 — the user has to scan the QR code again and again

Wanted: scan once per computer; after that the companion comes back by itself after an app restart, a rebuild, a
phone sleep or a Wi-Fi drop.

What the log shows (21:44 today, after a restart of the app):
- 21:44:38 the Mac announces its stored pairing `gs-34c0e5…` (stored key opened fine; it is AES-GCM sealed with the
  Keychain user key, so a wrong key cannot come out).
- The companion did not come back; the user scanned: 21:44:49 "a companion paired" (token path) — a **new** pairing,
  new pc id `04413604…`.
- Within 2 ms of the switch to the companion picture: "the companion's mirror link ended: the companion closed its
  mirror connection", the session falls back to the AOA control window, "the companion is off", the listener
  restarts on a new port, and 6 s later "a companion came back" with the new pairing and mirroring works.
- Same pattern at 21:10:50 → 21:10:54 ("paired" twice in 4 s).

Facts:
- The Mac keeps **one** pairing per phone (`companionPhone/Key/Pc` in `devices.json`); the companion keeps a
  **list** (`PairingStore`), tried in order by stored host:port, then by mDNS name `gs-<pc>`.
- The 21:09–21:10 refusals ("did not prove it holds the key") came right after the app's signature changed from ad
  hoc to the local certificate (`6be9581`); not yet explained either.

Find out (measure, don't guess — memory rule "research then measure"): why the companion closes its first mirror
link right after a fresh pairing; whether the two sides end up holding different pairings (compare the Mac's
`companionPc` with the companion's stored pcs: debug build, `adb shell run-as com.gamescrcpy.companion cat
shared_prefs/*.xml` — needs USB debugging on for a moment, ask the user); why the companion did not come back with
`34c0e5…` (backoff after the refusals? FAILED state? a pairing it never stored?). Fix so that one scan is enough and
the first mirror link after pairing does not drop. Add Mac unit tests where the logic allows (CompanionLan has a fake
companion in the session tests).

## Bug 2 — when mirroring stops, the session does not disconnect

The user: "投屏停止后不会自动断开" — when the mirroring stops (for example the user stops screen sharing on the phone,
the companion is stopped, or the mirror link ends), the Mac keeps the session/window up instead of disconnecting.
Reproduce, find which end it is (`CompanionMirror` reconnect wait `awaitChange` / "the companion did not come back
for mirroring", the session's reconnect logic, or the companion keeping its LAN link while the projection is gone),
and make it end cleanly: the window closes (or returns to the AOA control window if that is the intended fallback —
check `switchToCompanionPicture`'s comment and ask the user if unclear), the card shows the phone as not mirroring.

## Checks

`cmake --build --preset mac-app`, `ctest --preset mac-app -j8` (175 pass now), gradle
`JAVA_HOME=/opt/homebrew/opt/openjdk@17 ANDROID_HOME=~/Library/Android/sdk ./gradlew :companion:assembleDebug --offline`
in `android/`, then a run with the user on the phone. Before the user tests, build and open the newest
`build/mac-app/bin/anmacgamer.app` from **your** folder only if the user asks; otherwise tell them the commit to merge.
Quit the app with `osascript -e 'tell application id "io.github.wyb070110.anmacgamer" to quit'`.
