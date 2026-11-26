# Claude's backlog while the user is away (2026-11-25)

The user asked for an automation that keeps working. Each hourly run: take the first unfinished item, do a bounded
piece of it (about 30–45 min of work), keep `ctest --preset mac-app` green, commit locally with a clear message,
tick the item here, and add one line to `docs/handoff/daytime-claude.md`. Never quit or disturb the user's running
GameScrcpy: check UI in a demo copy with its own `GS_DATA_DIR` (`--demo-device tests/data/video/emulator_h264.media`).
Pushing to `ccr-e32722c2-75w11u` is fine to get CI to check a change (the user agreed to push the CI fix and to keep
working); never create releases, tags or anything else public without the user. No downloads, no brew installs.
Anything that needs the phone or the user: write it under "For the user" and move on.

## Items

- [x] **CI green.** (done 2026-11-25: run 45536621675 green after the macOS 15 voice fixes) Watch `gh run list -R wyb-070110/GameScrcpy-macos -L 3` for 2d72ad4 and later pushes; if a job
      fails, read `gh run view <id> --log-failed`, fix, push. Stop when the latest run is green.
- [x] **Merge Antigravity's A2/A5** (done 2026-11-25: merged 08378b0 with the user's OK, ctest 172/172) when `~/GameScrcpy-macos-antigravity/docs/handoff/antigravity.md` says 可合并 for
      them: review the diff (behaviour changes explained, no edits outside its tasks), merge, build, ctest, commit.
      Do not push the merge until CI on the previous push is green.
- [x] **UI polish, other screens.** (done 2026-11-25: solid base for non-main windows, ⌘ shortcuts, screenshots)
       Screenshot the settings page, profiles, tasks, extensions, the keymap editor and
      a dialog (dark and light) in the demo copy; fix what breaks the Liquid Glass look of the hub (opaque navy/old
      slabs, accent borders, cramped spacing, Windows leftovers). One commit per screen.
- [x] **Settings → Appearance.** (done: Blue first; Reduce transparency keeps the design)
       Show "Blue" (azure) first among the accent choices; check the "Reduce transparency"
      switch turns the glass off cleanly (screenshot both).
- [x] **Universal / older macOS plan.** (done: docs/handoff/older-macos-plan.md — found beta 1 ships GPL FFmpeg)
       Write `docs/handoff/older-macos-plan.md`: which bundled libraries need
      rebuilding from source for macOS 14+ and Intel (from the minos list in package.sh), where each comes from, size,
      build commands — the downloads need the user's OK, so only plan.

## For the user

- **Beta 2 is published** as a pre-release on anmacgammer (private), with the source on its main branch; beta 1's
  release on GameScrcpy-macos was deleted (tag kept). GitHub Actions is turned off for anmacgammer (the user
  agreed): CI runs on GameScrcpy-macos (public).

- Trackpad swipe feel (needs the phone): the four modes are in Settings → Touch → "Trackpad swipe mode".
