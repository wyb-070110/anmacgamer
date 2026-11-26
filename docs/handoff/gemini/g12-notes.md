# G12 Notes — Extension guides and examples: macOS first (2026-11-26)

## Files changed (26)

`docs/extensions/` (12 files):
- `docs/extensions/README.md`, `docs/extensions/README.zh-CN.md`
- `docs/extensions/manifest.md`, `docs/extensions/manifest.zh-CN.md`
- `docs/extensions/logic.md`, `docs/extensions/logic.zh-CN.md`
- `docs/extensions/native.md`, `docs/extensions/native.zh-CN.md`
- `docs/extensions/testing.md`, `docs/extensions/testing.zh-CN.md`
- `docs/extensions/publishing.md`, `docs/extensions/publishing.zh-CN.md`

`examples/extensions/` (14 files):
- `examples/extensions/deck-bridge/README.en.md`, `README.md`
- `examples/extensions/event-log/README.en.md`, `README.md`
- `examples/extensions/key-list/README.en.md`, `README.md`
- `examples/extensions/notes-panel/README.en.md`, `README.md`
- `examples/extensions/read-aloud/README.en.md`, `README.md`
- `examples/extensions/tour-and-help/README.en.md`, `README.md`
- `examples/extensions/voice-commands/README.en.md`, `README.md`

## What was replaced

- **CLI & binaries**: Replaced `gs.exe` ("next to the app") with `gs` inside `/Applications/anmacgamer.app/Contents/MacOS/gs` (and source-build path `build/mac-app/bin/anmacgamer.app/Contents/MacOS/gs`), adding a one-time `alias gs=/Applications/anmacgamer.app/Contents/MacOS/gs` note in `README` and `testing`. Replaced `gs-scripthost.exe` / `gs-exthost.exe` with `gs-scripthost` / `gs-exthost` (`Contents/MacOS`), and `GameScrcpy.exe` with `/Applications/anmacgamer.app/Contents/MacOS/anmacgamer`.
- **Paths & data directories**: Replaced `%APPDATA%\GameScrcpy\...` (and portable `data\...`) with `~/Library/Application Support/anmacgamer/...` (`logs`, `extensions`, `Containers`). Replaced Windows backslash paths (`.\`, `examples\extensions\...`, `samples\arena.keys`, `$env:USERPROFILE\keys\...`) with POSIX `/` paths and `~`. Replaced serial examples `COM7`/`COM9` with `/dev/cu.usbserial-0001` / `/dev/cu.usbserial-0002`.
- **Native builds & binaries**: Made `dylib` (Clang / Xcode Command Line Tools, universal `-arch arm64 -arch x86_64` or `{ "arm64": ..., "x64": ... }`, `__attribute__((visibility("default")))`) primary in `README`, `manifest`, `native`, `testing`, and `publishing`, keeping short trailing Windows `dll` / Visual Studio `/MT` notes where relevant for cross-platform authors. Preserved `docs/extensions/native.md#macos` and aligned the rest of the page with it.
- **Sandbox & crashes**: Rewrote restricted mode in `README`, `native`, and `testing` to lead with macOS `sandbox_init`, container `~/Library/Application Support/anmacgamer/Containers/GameScrcpy.Extension.<hash>`, no memory job limit, and `crash.txt` + logs (no minidump). Dropped Windows-only "Administrators / error 5" notes.
- **Shell & HTTP commands**: Changed `powershell` code blocks to `zsh`. Replaced `Invoke-RestMethod -Method Post` in `deck-bridge` with `curl -fsS -X POST`. Replaced `Get-FileHash` / `[uri]::EscapeDataString` in `publishing` with `shasum -a 256` and `python3` `urllib.parse.quote`.
- **Preserved intact**: All manifest fields (including `dll`, `dylib`, Windows reserved-name path validation rules), limits, API names, CLI flags, `gamescrcpy://` scheme, and error codes (`GS-API-*`, `GS_EXT_*`, `native-dll-invalid`, etc.).

## Items to note / double-check

- **`publishing.md` (`Install links`)**: Changed the portable-build bullet to state that macOS registers `gamescrcpy://` from `anmacgamer.app` (with a trailing note for Windows portable builds). Worth verifying `Info.plist` registers `gamescrcpy` scheme in the macOS bundle if not already confirmed.
- **`publishing.md` (URL encoding helper)**: Used `python3 -c 'import urllib.parse, sys; print(urllib.parse.quote(sys.argv[1], safe=""))'` alongside `shasum -a 256`. Works on macOS with Xcode Command Line Tools installed.
- **`testing.md` (`gs ext init native`)**: Kept `cmake -S . -B .build && cmake --build .build --config Release` as written (works on macOS with CMake + Clang).
- **`notes-panel/README*.md`**: Kept the literal UI button label `Add the newest GameScrcpy release` because it matches the string in `examples/extensions/notes-panel/src/main.ts`.
