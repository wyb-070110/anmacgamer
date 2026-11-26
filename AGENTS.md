# AGENTS.md

Guidance for AI coding agents (and humans) working in this repository. Read [CONTRIBUTING.md](CONTRIBUTING.md) for the full contributor guide and [docs/architecture.md](docs/architecture.md) for how the pieces fit together.

## What this is

**anmacgamer** mirrors an Android phone to macOS and maps keyboard, mouse, trackpad and gamepad input onto touches, with optional logic written as blueprints or TypeScript. The Mac side is C++20 / Objective-C++20 with Qt 6 / Qt Quick on Metal (`src/`), the phone side is a plain Java server and a Kotlin companion app (`android/`), and both speak the binary protocol specified in `protocol/SPEC.md`.

## Layout

- `src/` — the macOS app. Every module except `render`, `ui` and `app` is plain C++ / Objective-C++ without Qt and builds with the `posix-core` preset. `src/cli` is the `gs` command line.
- `android/` — `protocol`, `server` (plain Java, no Kotlin standard library) and `companion` (Kotlin).
- `protocol/` — the specification and the golden test vectors shared by C++ and Java.
- `sdk/` — generated script API types and docs (`gs api write sdk`), and the C header for native extensions.
- `tests/` — one doctest program per module, Qt Quick Tests for the interface, test data.
- `packaging/macos/` — self-contained `.dmg` packaging and Icon Composer scripts.
- `third_party/` — pinned dependencies; configuring never goes online.

## Build and test

```bash
cmake --preset mac-app
cmake --build --preset mac-app
cmake --build --preset mac-app --target all_qmllint
ctest --preset mac-app -R <regex>
```

- Presets: `mac-app` (local debug/dev app + tests), `mac-release` (universal `arm64` + `x86_64`, macOS 14+), `posix-core`, `posix-core-release`. Outputs land in `build/<preset>/bin`.
- QML lint must stay at zero warnings.
- Android: `cd android && ./gradlew build` (compiles, runs JUnit and lint; lint warnings are errors).
- The mirror window runs without a phone: `./build/mac-app/bin/anmacgamer.app/Contents/MacOS/anmacgamer --demo-device tests/data/video/emulator_h265.media` (set `GS_DATA_DIR` to isolate settings).
- Tests that need a phone run only when `GS_ADB_TEST_SERIAL` is set.

## Rules that the code base enforces

- `python3 tools/check_source_rules.py` must pass: no parsing of `dumpsys` / `wm` output, no fixed-delay synchronization (sleeps, delayed single-shot timers, `postDelayed`; a deliberate delay carries `source-rule-allow R9: <reason>` on the same line), every QML `Text` declares `textFormat`, controls in `src/ui/controls` take colors, sizes and durations only from the theme, and every user-facing string is English in code and translated in `src/app/i18n/anmacgamer_zh_CN.ts`.
- After changing interface text, run the translation update target (`anmacgamer_lupdate`) and translate the new entries.
- Protocol changes go through `protocol/SPEC.md` and `protocol/tools/gen_vectors.py`; never edit the generated vectors by hand. Both the C++ and the Java tests must pass.
- After changing the script API table in `src/script/src/Api.cpp`, regenerate `sdk/` with `./build/mac-app/bin/gs api write sdk`.

## Conventions

- Let the code speak for itself: comments are kept to a short description at the top of a file; explanations belong in pull requests and documentation.
- Identifiers and commit messages are English; user-facing documentation is Simplified Chinese first, with English versions of the main documents.
- Interface colors, type sizes, spacing, radii and motion come from `src/ui/theme`, never bare numbers.
- New behavior comes with tests; a bug fix starts with a failing test; new macOS port files carry `SPDX-License-Identifier: GPL-3.0-or-later`.
- Do not add network downloads to configuring or building; new dependencies are vendored into `third_party/` or built reproducibly via `tools/build_macos_deps.sh`, with versions, licenses and checksums recorded in `third_party/README.md` and `LICENSING.md`.
