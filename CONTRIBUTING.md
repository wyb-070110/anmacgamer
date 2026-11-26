# Contributing to anmacgamer

English · [简体中文](CONTRIBUTING.zh-CN.md)

## Report an issue

- Search [Issues](https://github.com/wyb-070110/anmacgamer/issues) first.
- Include the app version, macOS version and chip, phone model and Android version, connection type, steps to reproduce, and the archive from **Settings > About > Export diagnostics**.
- Report security issues privately as described in [SECURITY.md](SECURITY.md).

## Set up and build

Requirements: macOS 14+ (Apple silicon or Intel), Xcode 16+ (Xcode 26+ to recompile `.icon` assets), Qt 6.10.3 at `~/Qt/6.10.3/macos`, CMake 3.28+, Ninja, Python 3.10+. For Android changes: JDK 17 and Android SDK platforms 35/36.

```bash
brew install cmake ninja openssl@3 ffmpeg sdl3 libusb android-platform-tools
cmake --preset mac-app
cmake --build --preset mac-app
ctest --preset mac-app
```

CMake configure must work offline (no `FetchContent` or build-time downloads). Pass `-DCMAKE_PREFIX_PATH=<path>` if Qt is elsewhere. Outputs go to `build/<preset>/bin`.

| Preset | Purpose |
|:--|:--|
| `mac-app` | Local dev build (`anmacgamer.app`, `gs`, tests) |
| `mac-release` | Universal (`arm64` + `x86_64`) macOS 14+ release build |
| `posix-core` | Qt-free core libraries and tests (Debug) |
| `posix-core-release` | Optimized core libraries and benchmarks |

Android (`server` must stay plain Java; versions live in `android/gradle/libs.versions.toml`):

```bash
cd android && ./gradlew build
```

## Test before opening a PR

- Unit tests: `ctest --preset mac-app` (filter with `-R <regex>`). Device tests run only when `GS_ADB_TEST_SERIAL` is set.
- QML lint (0 warnings required): `cmake --build --preset mac-app --target all_qmllint`.
- Source rules: `python3 tools/check_source_rules.py` (blocks `dumpsys`/`wm` parsing [R5], blocks fixed-delay sync like `sleep`/`postDelayed` unless tagged `source-rule-allow R9: <reason>`, checks English source strings with translations, theme-only tokens in `src/ui/controls`, and `textFormat` on QML `Text`).
- Demo window without a phone: `./build/mac-app/bin/anmacgamer.app/Contents/MacOS/anmacgamer --demo-device tests/data/video/emulator_h265.media` (set `GS_DATA_DIR` to isolate settings).

## Repository layout

| Path | Contents |
|:--|:--|
| `src/` | macOS app (C++20 / ObjC++20, Qt 6 / Metal) and `gs` CLI; see [docs/architecture.md](docs/architecture.md) |
| `android/` | `protocol`, `server` (Java), `companion` (Kotlin) |
| `protocol/` | `SPEC.md` and shared golden test vectors |
| `sdk/` | Generated script API docs/types and native C header |
| `tests/`, `examples/`, `tools/`, `packaging/macos/`, `third_party/` | Tests, sample extensions, build/check scripts, `.dmg` packaging, vendored deps |

## Specific change workflows

- Protocol changes: edit `protocol/SPEC.md`, update `protocol/tools/gen_vectors.py`, run `python3 protocol/tools/gen_vectors.py`, and pass both C++ and Java tests. Never hand-edit generated vectors.
- Script API changes: after editing `src/script/src/Api.cpp`, run `./build/mac-app/bin/gs api write sdk` from repo root.
- UI strings and help: write English in `tr()`/`qsTr()`, run `anmacgamer_lupdate`, and update `src/app/i18n/gamescrcpy_zh_CN.ts`. Built-in help lives in `src/app/help/` (`index.json`, `en/`, `zh_CN/`, registered in `src/app/CMakeLists.txt`).
- Packaging a release: run `tools/build_macos_deps.sh`, build `mac-release`, then run `packaging/macos/package.sh`. Rebuild icons with `packaging/macos/icon/build_icon.sh` (Xcode 26+).

## Submit a pull request

1. Fork and branch from `main`. Open an issue first for large features, protocol changes, or profile format changes.
2. Keep each PR to one focused change with tests (start bug fixes with a failing test).
3. Format C++20/ObjC++20 with `.clang-format` and `.editorconfig`. Pull UI tokens from `src/ui/theme`. Keep inline comments minimal.
4. Write English commit messages (first line <= 72 chars). Describe the change and how you tested it in the PR.

## License

New files and macOS port code are contributed under [GNU GPL 3.0 or later](LICENSES/GPL-3.0-or-later.txt) (add `SPDX-License-Identifier: GPL-3.0-or-later` at the top of new files). Changes to upstream files keep their original license; see [LICENSING.md](LICENSING.md). Community participation follows the [Code of Conduct](CODE_OF_CONDUCT.md).
