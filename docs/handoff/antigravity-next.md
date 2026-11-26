# Antigravity — next batch (2026-11-25 midday, set by Claude before handing over)

Claude is out of quota; you carry on alone until it is back. Rules as before (TEAM.md): work only in
`~/GameScrcpy-macos-antigravity` on `mac/antigravity`; no push, no downloads, no brew installs, no Overages; never quit
the user's running GameScrcpy (test with a demo copy and its own `GS_DATA_DIR`); one commit per task; keep
`ctest --preset mac-app` green; log in `docs/handoff/antigravity.md` and mark each task 可合并 when done.

**First:** merge the mainline into your branch: `git merge ccr-e32722c2-75w11u` from `~/GameScrcpy-macos`
(fetch it with `git fetch ~/GameScrcpy-macos ccr-e32722c2-75w11u`). It already contains your A2/A5 (trial merge
reviewed by Claude), the touch timing fix, the icon pipeline and Gemini's icon proposals.

## A6 — The app icon: Gemini's direction B (the user's choice)

Spec: `docs/handoff/gemini/icon.md`, section 方向 B「双屏跃迁与极速帧流（Dual-Frame Stream）」. Gemini only drew
layers for direction A (`docs/handoff/gemini/icon/layers/`): do not use those.

1. Draw B's layers as 1024×1024 full-bleed canvases (transparent PNG or SVG; keep everything inside 164…860 px so the
   system mask never cuts it): back = navy portrait phone `#1D3557`→`#0F2038`, rounded; front = larger landscape
   glass screen `#0A84FF`→`#38BDF8`, overlapping it to the right; on the front screen a bold white right chevron (›).
   Extend `packaging/macos/icon/make_icon.swift` (it already has a `layers` mode and the CoreGraphics helpers) or
   write SVGs by hand.
2. Rewrite `packaging/macos/icon/GameScrcpy.icon/icon.json` and `Assets/`: background
   `"fill": {"linear-gradient": ...}` or `"automatic-gradient"` close to `#F0F6FF`→`#CFE4FF`; one group per layer,
   `"glass": true` on the front screen. Check the JSON keys against Xcode's template
   (`/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/Library/Xcode/Templates/File Templates/macOS/Resource/Icon Composer Icon.xctemplate`)
   and open it in Icon Composer (`/Applications/Xcode.app/Contents/Applications/Icon Composer.app`) to see it.
3. Compile: `xcrun actool packaging/macos/icon/GameScrcpy.icon --compile <dir> --platform macosx
   --minimum-deployment-target 14.0 --app-icon GameScrcpy --output-partial-info-plist <dir>/partial.plist`. This gives
   `Assets.car` and `GameScrcpy.icns`.
4. Ship both: replace `src/app/resources/GameScrcpy.icns`, add `src/app/resources/Assets.car` the same way the
   `.icns` is added in `src/app/CMakeLists.txt` (`MACOSX_PACKAGE_LOCATION Resources`), and add
   `<key>CFBundleIconName</key><string>GameScrcpy</string>` to `src/app/Info.plist.in`. Better still, make CMake run
   actool at build time from the `.icon` (a custom command) so nobody commits binaries by hand — your call.
5. Check: build, then render the bundle's icon as Finder sees it
   (`NSWorkspace.shared.icon(forFile:)` in a small Swift script, see how Claude did it in
   `docs/handoff/daytime-claude.md`) — on macOS 27 it must NOT sit on a grey plate. Also look at 16/32 px. Put
   the renders in `docs/handoff/glass/icon-b-*.png`. Update `packaging/macos/icon/GameScrcpy-1024.png`.

## A7 — If time is left: touch trace tool

`GS_TOUCH_TRACE=<file>` (see `src/app/ControlChannel.cpp`) logs the touch frames sent. Write
`tools/touch_trace.py` (like `tools/frame_trace.py`): per gesture, the moves' spacing in ms, the speed over the last
100 ms before the lift, and the gap between the last move and the lift (Android treats > 40 ms as a stopped finger;
see `docs/handoff/swipe-design.md`). Test it on a trace from a demo copy; do not touch the user's phone.

## NOW (2026-11-25 evening): clean up, then A9

1. **Clean up first.** Throw away your uncommitted A8 copy (`git checkout -- src tests`, delete the untracked
   `docs/handoff/glass/toolbar-*.png`). Then `git fetch ~/GameScrcpy-macos ccr-e32722c2-75w11u` and
   `git merge FETCH_HEAD`: the mainline now has your rename, icon B, DMG icon and touch_trace.py merged (af89b52),
   plus two doc fixes by Claude (the ESP32 download link and the QtScrcpy credit stay). Check
   `ctest --preset mac-app` is green and log it.

## A9 — The app must quit cleanly on SIGTERM (and SIGINT, SIGHUP)

`kill -TERM <pid>` (what `killall`, launchd and logout send) does not make GameScrcpy quit. It has to: a clean quit
is what switches the phone back to its own keyboard, stops the phone-side server and the companion, and writes
settings.

1. Reproduce on a **demo copy only** (never the user's running GameScrcpy at
   `~/GameScrcpy-macos/build/mac-app/bin/GameScrcpy.app`, pid from `pgrep -f`; do not signal it): run the built app
   with its own `GS_DATA_DIR` and `--demo-device tests/data/video/emulator_h264.media`, send it SIGTERM, see whether
   it leaves and what the log (`$GS_DATA_DIR/logs/gamescrcpy.log`) says.
2. Find why: look in `src/app/main.cpp` and `src/platform` for any signal handling (POSIX `sigaction`, a blocked
   signal mask in a worker thread, `SIG_IGN`), and at how Windows asks the app to quit (`WM_CLOSE` /
   console-control handler) so macOS does the same thing.
3. Fix with the usual safe pattern: a self-pipe or `socketpair` written from an async-signal-safe handler (or
   `dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, …)` with the signal ignored by default), read on the Qt
   main thread with a `QSocketNotifier`, which then calls the same path as Quit in the menu
   (`QCoreApplication::quit` or the app's own quit function, whatever ⌘Q uses). A second signal while quitting may
   exit at once. Keep it in the macOS/POSIX platform code; do not change Windows behaviour.
4. Test: a unit or integration test if one is practical (e.g. start a small helper, send SIGTERM, check the exit
   code), and by hand on the demo copy: SIGTERM → the log shows a normal exit with code 0 within a couple of seconds.
5. One commit, log in `docs/handoff/antigravity.md`, mark 可合并, then stop.

## A8 — DONE BY CLAUDE (2026-11-25 19:05, commit 88dad52): your quota ran out mid-task

Claude reviewed your uncommitted A8 work and took it into the mainline (separators, More-menu entries, grid axes,
compact chevron, the QML tests in PortProbe.cmake and your toolbar-*.png captures), all but the `GS_A8_GRAB` hook in
`src/app/main.cpp` (a test hook does not belong in the shipped app). When your quota is back: throw away your
uncommitted A8 changes (`git checkout -- src tests` and remove the untracked `docs/handoff/glass/toolbar-*.png`),
merge `ccr-e32722c2-75w11u`, check `ctest --preset mac-app` is green, log it in antigravity.md, and stop: there
is no next item until Claude or the user adds one.

## A8 (original task, for reference)

Claude changed the mirror toolbar (commits `ba65956`, `4a4473c` on `ccr-e32722c2-75w11u`; merge the mainline first):
`src/ui/controls/FloatingToolbar.qml` now keeps its buttons in one `Grid` (a row, or one column when
`vertical`) instead of moving them between a Row and a Column; `ToolbarSeparator.qml` lies across when its parent
grid has one column; `src/app/qml/MirrorOverlay.qml` has More → "工具栏按钮…" (`toolbarButtonsMenu`): check marks
take buttons off the toolbar (they then appear in the More menu) and an "添加到工具栏" section adds More-menu
commands (`toolbarExtras`); both are stored in the setting `display.toolbarHidden` (comma-separated ids, added
commands start with `+`). The bug fixed: on a side toolbar, buttons hidden when they were moved were never drawn
once shown (an empty gap in the toolbar).

1. **Automated:** get `tests/ui/controls/tst_floating_toolbar.qml` (and `tst_toolbar_reveal.qml`) running in the
   macOS build (see how `tests/CMakeLists.txt` runs the QML tests on Windows; add them to `tests/PortProbe.cmake`).
   Add cases: a vertical toolbar whose button starts hidden and is shown later gets a real size and position
   inside the toolbar; switching `vertical` back and forth keeps the order of the buttons; a separator lies across
   in a vertical toolbar and stands up in a horizontal one. Keep `ctest --preset mac-app` green.
2. **By eye, in a demo copy only** (never the user's running GameScrcpy): a fresh `GS_DATA_DIR` with a
   `settings.json` such as
   `{"format":"gamescrcpy.settings","version":1,"general":{"language":"zh_CN"},"display":{"toolbar":"pinned","toolbarSide":"right","toolbarHidden":"diagnostics,+device.files,+device.notifications"}}`,
   started with `--demo-device tests/data/video/emulator_h264.media`. Note: the app rewrites `settings.json` and
   drops values equal to the default, so write a fresh file for every run. Capture the mirror window with
   `screencapture -x -o -l <window id>` (window ids of the demo's pid via `CGWindowListCopyWindowInfo`, see how
   Claude did it). Check all four sides (top, bottom, left, right), a tall and a short window, buttons taken off
   and added, and "恢复默认" (an empty `toolbarHidden`). Put the captures in `docs/handoff/glass/toolbar-*.png`.
3. Report what you found in `docs/handoff/antigravity.md` (bugs with steps; fix small ones in their own commit,
   leave bigger ones described for Claude), mark 可合并.
