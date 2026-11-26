# G12 — Extension guides and examples: macOS first (2026-11-26)

After G11 the extension guides still show the upstream Windows commands and paths: `gs.exe`, `gs-scripthost.exe`,
`gs-exthost.exe`, `GameScrcpy.exe`, `%APPDATA%\GameScrcpy\...`, PowerShell (`Invoke-RestMethod`), Visual Studio and
`/MT`, `.dll`, AppContainer, job objects. About 89 places. anmacgamer is a macOS app: make these pages macOS first.

## Files

`docs/extensions/*.md` and their `*.zh-CN.md` twins (README, logic, manifest, native, publishing, testing), and
`examples/extensions/*/README.md` and `README.en.md`. Nothing else (no `src/`, no `src/app/help/`, no `sdk/`, no
`protocol/`, no `docs/handoff/` except your notes).

## macOS facts to use (checked by Claude)

| Windows text | macOS |
|---|---|
| `gs.exe` "next to the app" | `gs`, inside the app: `/Applications/anmacgamer.app/Contents/MacOS/gs` (a source build: `build/mac-app/bin/anmacgamer.app/Contents/MacOS/gs`). Suggest `alias gs=/Applications/anmacgamer.app/Contents/MacOS/gs` once in a shell profile. |
| `gs-exthost.exe`, `gs-scripthost.exe` | `gs-exthost`, `gs-scripthost` in the same folder (`Contents/MacOS`) |
| `GameScrcpy.exe` | `anmacgamer.app` (the app; executable `Contents/MacOS/anmacgamer`) |
| `%APPDATA%\GameScrcpy\` | `~/Library/Application Support/anmacgamer/` (folders: `extensions`, `profiles`, `components`, `tasks`, `logs`, `crashes`, `Containers`) |
| `.dll`, Visual Studio, `/MT`, `cl.exe` | `.dylib` built with Clang (Xcode Command Line Tools), universal (`-arch arm64 -arch x86_64`) or per-arch paths `{ "arm64": ..., "x64": ... }`; export `gs_ext_main` with `__attribute__((visibility("default")))` (the `GS_EXT_EXPORT` macro). `dll` entries are ignored on macOS; a package may ship both. |
| AppContainer, job object, 1 GiB limit | `"sandbox": "restricted"` uses macOS `sandbox_init`: read-only own folder and system libraries, read/write container `~/Library/Application Support/anmacgamer/Containers/GameScrcpy.Extension.<hash>` (also `HOME` and `TMPDIR`), no network, no child processes; no memory job limit. (The container name really still starts with `GameScrcpy.Extension.`.) |
| Minidumps | crashes write `crash.txt` (signal, address, library, offset) and logs; no minidump |
| PowerShell `Invoke-RestMethod` | `curl` (for example `curl -fsSL <url> -o file.gsext`), `zsh` syntax |
| Windows paths with `\` | POSIX paths with `/`; `~` for the home folder |
| "Administrators, error 5" notes | drop them (Windows only) |

The detailed macOS section that already exists (`docs/extensions/native.md#macos`) is correct; keep it and make the
rest of the page agree with it. Where a Windows detail matters for authors who also ship to Windows (`dll` next to
`dylib`), keep one short "Windows" note at the end of the section instead of leading with it.

## Rules

- Keep every manifest field, rule, limit, API name, CLI command and error code. Only the platform parts change.
- Commands must be ones that work on macOS as written; if unsure, keep the command and note it in your notes.
- English first; the `.zh-CN.md` twin says the same.
- Same style as G11: short, plain, no marketing words.

## Deliver

Edit the files directly, do not commit. Notes to `docs/handoff/gemini/g12-notes.md`: files changed, what you
replaced, anything unsure. Work in the foreground, one file at a time (background subtasks cannot reach the folder).
