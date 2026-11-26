# Testing extensions

English · [简体中文](testing.zh-CN.md)

Use `gs` (inside `/Applications/anmacgamer.app/Contents/MacOS/gs`, or `build/mac-app/bin/anmacgamer.app/Contents/MacOS/gs` in a source build) to validate, type-check, and test extensions without running anmacgamer or connecting a phone. Add `alias gs=/Applications/anmacgamer.app/Contents/MacOS/gs` to `~/.zshrc` once so the commands below work as written.

| Command | Purpose |
|:--|:--|
| `gs validate <folder \| pkg.gsext \| bundle.gsbundle> [--json]` | Check manifest, files, filter shaders, profiles, signature, and dylib arch/`gs_ext_main` |
| `gs check <folder> [--json]` | Type-check TypeScript and run script rules |
| `gs types <folder>` | Write `types/gamescrcpy.d.ts` and `types/extensions.d.ts` for editors |
| `gs ext test <folder \| pkg.gsext> [--test <file>]... [--hosted] [--json]` | Load manifest, dylib, and scripts in mock host and run `tests/*.json` |
| `gs ext init <native \| script \| theme> <folder> [--id <id>]` | Scaffold a minimal extension with a test |
| `gs ext <id \| folder \| pkg.gsext> <cmd> [--<param> <val>]... [--hosted] [--json]` | Run a `contributes.cli` command |

## `gs validate`

```zsh
gs validate ./my-extension
gs validate ./alice.lap-timer-1.2.0.gsext --json
```

Runs the installer checks: manifest fields, paths, sizes, filter shader compilation (`--compile-timeout <seconds>`, default 20 s), profiles/patches, signatures, and each dylib in `contributes.native.dylib` (and `dll` if shipped: architecture and `gs_ext_main` export). Exit codes: `0` ok, `1` errors, `2` bad CLI arguments.

## `gs check` and `gs types`

- `gs check <folder>` runs the bundled TypeScript compiler on all `.ts` files (skipping `types/`, `node_modules`, and dot-prefixed folders), blocking non-erasable syntax (`enum`, decorators) and unsafe patterns (loops without `await`, fixed `sleep` before vision). Requires `tsconfig.json` and `src/main.ts` (or `src/main.js`). Prints `no problems` on success.
- `gs types <folder>` writes `types/gamescrcpy.d.ts` and `types/extensions.d.ts` (nodes of installed native extensions). Re-run after updating anmacgamer.

## The mock host: `gs ext test`

```zsh
gs ext test ./my-extension
gs ext test ./my-extension --hosted
gs ext test ./alice.lap-timer-1.2.0.gsext --test ./more-tests/flash.json --json
```

Execution flow:
1. Validates with `gs validate` (aborts with exit code `2` on errors).
2. Loads each script for 2 s of test time against a mock phone (`ui.*` is recorded; scripts needing unmocked services like network, files, local listen, or speech are marked `limited`; script errors are marked `failed`).
3. Loads the dylib in-process by default, or in `gs-exthost` (inside `Contents/MacOS`) with `--hosted`. Restricted dylibs (`"sandbox": "restricted"`) always run in `gs-exthost` under macOS `sandbox_init` (`gs-test:<id>`).
4. Runs `tests/*.json` in name order (or `--test` files) with a fresh dylib, default settings, and all manifest permissions.

Exit codes: `0` all passed, `1` step failed or no tests, `2` load failed.

### Test files

UTF-8 JSON, max 1 MiB, max 64 nesting levels:

```json
{
  "format": "gamescrcpy.ext-test",
  "version": 1,
  "name": "flashing",
  "granted": ["hardware"],
  "settings": { "speed": 115200 },
  "deviceSettings": { "mock-phone": { "port": "/dev/cu.usbserial-0002" } },
  "holders": [ { "resource": "serial:/dev/cu.usbserial-0001", "by": "AOA session", "yields": true } ],
  "timeout": 20000,
  "steps": [
    { "do": "call", "node": "version", "expect": { "value": "1.0.2" } },
    { "do": "job", "job": "flash", "args": { "port": "/dev/cu.usbserial-0001" },
      "expect": { "result": { "written": true }, "progress": [ { "text": "erasing" }, { "fraction": 1 } ] } }
  ]
}
```

| Field | Meaning |
|:--|:--|
| `format`, `version` | Optional; if set, `gamescrcpy.ext-test` and `1` |
| `name` | Report name (default: filename) |
| `granted` | Subset of manifest permissions to grant (default: all) |
| `settings`, `deviceSettings` | Initial settings; mock phone id is `mock-phone` |
| `holders` | Pre-existing hardware holders: `{ resource, by, yields }` |
| `timeout` | Per-step wait in ms (1–600000, default 10000) |
| `steps` | 1–1000 steps (`do`, optional `name`, `expect`, per-step `timeout`) |

### Steps

| `do` | Input | `expect` keys |
|:--|:--|:--|
| `call` | `node`, `args` (array) | `value`, `error` |
| `job` | `job`, `args` (object), `id`, `wait` (default `true`) | `result`, `error`, `progress`, `logs`, `outputs` |
| `job.cancel` / `job.wait` | `id` | `ended` / `result`, `error`, `progress`, `logs`, `outputs` |
| `touch.open` | `method`, `session`, `phone`, `device`, `touchBox`, `options` | `opened` |
| `touch.frame` | `session`, `contacts` (`{ id, action, x, y, … }`, `action`: `down`/`move`/`up`/`cancel`), `rotation` | `ok`, `sent` |
| `touch.key` | `session`, `key` or `keycode`, `down`, `repeat`, `meta` | `ok` |
| `touch.text` | `session`, `text` | `ok` |
| `touch.mouse` | `session`, `buttons`, `dx`, `dy`, `wheel` | `ok` |
| `touch.pad` | `session`, `buttons`, `hat`, `leftX`, `leftY`, `rightX`, `rightY`, `leftTrigger`, `rightTrigger` | `ok` |
| `touch.close` | `session` | — |
| `devices` | `provider` | `devices`, `count`, `error`, `problems` |
| `device.act` | `provider`, `device`, `action` | `ok`, `error` |
| `events` | — | Emits events since last `events` step |
| `settings` | `settings`, `deviceSettings` | `changed` (keys passed to `settings_changed`) |
| `wait` | `ms` | Advances test clock (including in-process `now_us`) |
| `holder` / `holder.release` | `resource`, `by`, `yields` / `resource` | — / `released` |
| `claim.want` / `claims` | `resource`, `by` / — | `held`, `yielded`, `released` / `held`, `holders` |
| `scripts` / `manifest` | — | `loaded`, `logs` / `value` |

Every step also supports in-order checks for `events` (`{ topic, value }`), `logged` (`{ level, text }` or substring), `notified` (`{ title, text }` or substring), and `devicesChanged` (provider names). Matching rules: objects match listed keys only; arrays match length and elements; numbers allow tiny tolerance; `error` accepts `true`, `false`, or substring; `progress`/`logs`/`outputs` (from `job_output`) match in order with substring text matching.

For script extensions, test loading and logs:

```json
{ "steps": [ { "do": "scripts", "expect": { "loaded": ["main"], "logs": ["ready"] } } ] }
```

## Starting points: `gs ext init`

```zsh
gs ext init script ./lap-timer --id alice.lap-timer
gs ext init native ./board-tools
gs ext init theme ./night-sky
```

Creates an empty-folder starter with `AGENTS.md` and `tests/basic.json` (default id: `my.<folder>`; `gamescrcpy.` rejected):
- `script`: `src/main.ts` (main-window page), `tsconfig.json`, `types/gamescrcpy.d.ts`.
- `native`: CMake project, `include/gamescrcpy_ext.h`, `src/native.cpp` (`greet` node, `countdown` job), `contributes.cli`. Build with `cmake -S . -B .build && cmake --build .build --config Release`.
- `theme`: dark and light color tokens.

## Command-line commands

`contributes.cli` exposes native jobs as CLI commands ([manifest reference](manifest.md#cli-up-to-16)):

```zsh
gs ext ./board-tools --help
gs ext ./board-tools countdown --from 5
gs ext alice.board flash --port /dev/cu.usbserial-0001 --json
```

Target may be a folder, `.gsext`, or installed extension id. Pass `--name value` or `--name=value` (booleans `true`/`false`, numbers, points `x,y`, regions `x,y,width,height`, `json`; omitted values use manifest `default`). Prints `[ 40%] text`, indented logs, and final JSON (`--json` for report; Ctrl+C cancels; `--hosted` runs in `gs-exthost`). Exit codes: `0` ok, `1` job failed, `2` load/CLI error.

## Trying it in anmacgamer

1. Run `gs pack <folder>` ([Packing and publishing](publishing.md)), install via **Install from file…**, enable, and approve permissions.
2. Re-pack and re-install to overwrite even the same version. Installed path: `~/Library/Application Support/anmacgamer/extensions/<id>/<version>/`.
3. Check logs (`~/Library/Application Support/anmacgamer/logs`) for `[extensions]` and `logic …` lines, and check card Details.
4. Test the mirror window without a phone: `/Applications/anmacgamer.app/Contents/MacOS/anmacgamer --demo-device tests/data/video/emulator_h265.media`.
