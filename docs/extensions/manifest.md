# Manifest reference

English · [简体中文](manifest.zh-CN.md)

`manifest.json` sits at the root of an extension folder: UTF-8 JSON object, max 256 KiB, max 32 nesting levels. `gs validate` and anmacgamer enforce the same rules (`--json` lists code, JSON path, and fix hint). Errors block installation; warnings (like `unknown-field`) do not.

## Minimal example

```json
{
  "schemaVersion": 1,
  "id": "alice.lap-timer",
  "version": "1.2.0",
  "gamescrcpy": "2.1",
  "name": "Lap timer",
  "description": "Times laps and shows them in a main-window page.",
  "authors": ["Alice"],
  "license": "MIT",
  "repository": "https://github.com/alice/lap-timer",
  "requires": { "script": "^1.1", "panel": "^1.0", "help": "^1.0" },
  "dependencies": { "alice.common": "^2.0" },
  "contributes": {
    "scripts": [
      { "name": "main", "entry": "src/main.ts", "run": "app" },
      { "name": "phone", "entry": "src/phone.ts", "run": "session" }
    ],
    "panels": [
      { "name": "laps", "title": "Laps", "icon": "timer", "where": "page", "script": "main" }
    ]
  },
  "settings": [
    { "key": "sound", "type": "boolean", "title": "Beep on each lap", "default": true }
  ],
  "permissions": [
    { "name": "mapping", "reason": "Listens to the lap key of the active profile." }
  ],
  "l10n": { "zh_CN": { "name": "圈速计时", "panels.laps.title": "圈速" } }
}
```

## Top-level fields

| Field | Req | Rule |
|:--|:--|:--|
| `schemaVersion` | yes | Always `1` |
| `id` | yes | 2+ dot-separated segments of `[a-z0-9-]` (no leading/trailing/double `-`), max 64 chars (`gamescrcpy.*` reserved) |
| `version` | yes | Semver `major.minor.patch` (optional prerelease tag, e.g. `1.2.0-beta.1`) |
| `gamescrcpy` | | Target app `major.minor` or `major.minor.patch` (no prerelease); see [Which version it was written for](#which-version-it-was-written-for) |
| `name`, `description` | `name` | Max 100 / 1000 chars |
| `authors` | | Up to 32 names, max 100 chars each |
| `license` | yes | License string (prefer SPDX like `MIT`, `Apache-2.0`), max 100 chars |
| `repository` | | `https://` URL, max 512 chars |
| `authorKey` | | Minisign public key (56 chars starting with `RW`); see [Packing and publishing](publishing.md) |
| `updates` | | `{ "index": "https://…" }`; see [`updates`](#updates) |
| `aiAssisted` | | `true` if built with AI assistance |
| `requires` | yes | Extension points and semver ranges (at least one) |
| `dependencies` | | Extension id -> range or `{ "version": range, "optional": true }` |
| `contributes` | | Extension contributions |
| `settings`, `permissions` | | Up to 128 settings; up to 32 permissions |
| `l10n` | | Translated strings per locale |

### Name formats

| Format | Rule | Used for |
|:--|:--|:--|
| Native name | `[a-z][a-zA-Z0-9]*`, max 64 chars | Native `name`, node/job params, services, attachment/behavior param `key` |
| Panel name | 1–32 `[a-z0-9-]` | Panels, help/tour `id`, scripts, converters, `files.paths` folder names |
| Profile name | 1–64 `[a-zA-Z0-9_-]` | Profiles, components (max 48), tasks |
| Setting key | `[a-zA-Z][a-zA-Z0-9]*`, max 64 chars | `settings[].key` |
| Package path | Relative, `/`-separated, inside folder, no `\ : * ? " < > \|`, max 260 chars (segment max 255, no trailing `.`/space, no Windows names like `con`, `nul`, `com1`) | `file`, `entry`, `folder`, `dll`, `dylib` |

Dot-prefixed files/folders (`.git`, `.build`) are neither checked nor packed.

### `requires` and version ranges

Ranges: `^1.0` (`>=1.0 <2.0`, or minor-locked on `0.x`), `~1.2` (`>=1.2 <1.3`), space-separated comparisons (`>=1.0.0 <2.0.0`), exact (`1.2.3` / `=1.2.3`), or `*`. Prerelease versions only match ranges naming a prerelease of the same version.

Minor bumps add features (backward-compatible); major bumps break compatibility; tightening a single field (like `files.paths` `paths`) warns without a major bump.

| Extension point | 2.0.0 | 2.1.0 | 2.2.0 | Notes |
|:--|:--|:--|:--|:--|
| `filter`, `theme`, `profile` | 1.0 | 1.0 | 1.0 | — |
| `script`, `component`, `task` | 1.0 | 1.1 | 1.1 | 2.1.0: `net`, `files`, `speech`, `events`, `media`, `ext.provide`/`ext.call`, converters, new `ui.*` |
| `panel` | 1.0 | 1.1 | 1.1 | 2.1.0: `device`, `settings`, `extension`, `toolbar`, `contextMenu`, `tray`, `on` |
| `native` | 1.0 | 2.0 | 2.1 | 2.1.0: interface 2 (`gs_ext_main`, jobs, providers, media, AI, restricted); 2.2.0: AI `xhigh`/`max`, `topEffortsThink`, `millionContext` ([Native guide](native.md)) |
| `help`, `tour`, `locale` | — | 1.0 | 1.0 | Added in 2.1 |
| `command`, `preset` | — | — | — | Reserved, not yet offered |

Compatibility results (`gs validate`, `gs pack`, and extension cards):
- `host-api-missing`: program lacks extension point ("Needs a newer anmacgamer").
- `host-api-version`: requested version is newer ("Needs a newer anmacgamer").
- `host-api-outdated`: all offered versions exceed range ("Made for an older anmacgamer"; never triggered by lower-bound-only ranges like `>=1.0`).

### Which version it was written for

Optional `"gamescrcpy": "2.1"` (`major.minor` or `major.minor.patch`, no prerelease; invalid -> `version-invalid`). Used only for display ("Made for anmacgamer 2.1"); compatibility always comes from `requires`. If omitted, inferred from `requires`; `gs validate` and `gs pack` warn `gamescrcpy-missing` (loading does not).

### `dependencies`

Map of `<extension id>` to semver range or `{ "version": "<range>", "optional": true }`. Missing/disabled/incompatible required dependencies keep the extension off. Controls load order and allowed cross-extension service/event targets ([Logic guide](logic.md)). Self-dependency is `dependency-cycle`.

### `updates`

`{ "index": "https://…" }`: `https://` with host, no username, printable ASCII, max 2048 chars (else `url-invalid`). Fetched only on manual check or daily startup check (off by default), without sending extension `id`. Format: [Update lists](publishing.md#update-lists).

## `contributes`

Every contribution requires its extension point in `requires` (else `contribution-undeclared`). `name`/`id` must be unique within each kind.

### `themes` (`theme`, max 32)

`{ "name": "Slate", "dark": { "bgBase": "#0E1116" }, "light": { "bgBase": "#E6EAF1" } }`. `name` <= 64 chars; token `[a-z][a-zA-Z0-9]*`; color `#RRGGBB` or `#RRGGBBAA`. Example: [slate-theme](../../examples/extensions/slate-theme).

### `filters` (`filter`, max 32)

`{ "name": "Scanlines", "file": "shaders/scanlines.hlsl" }` (`.hlsl` <= 1 MiB). Directives:
- `//! PARAM <name> <float|int|bool> <default> [<min> <max> [<step>]]` (max 64; reads same-key `settings`)
- `//! PASS <name> [scale=<ratio>] [format=rgba8|rgba16f]` (1–16; `float4 <name>(float2 uv)`, `0 < scale <= 4`)
- `//! TEXTURE <name> <file.png>` (max 8)

Built-ins: `Original`, `Source`, `Linear`, `Point`, `SourceSize`, `OriginalSize`, `OutputSize`, `Frame` (`Gs*` reserved). Example: [crt-scanlines](../../examples/extensions/crt-scanlines).

### `profiles` and `profilePatches` (`profile`, max 256 each)

- `profiles`: `{ "name", "file" }` (profile name, `gamescrcpy.profile` JSON <= 1 MiB).
- `profilePatches`: `{ "target": "<ext-id>/<profile>", "file" }` (`"format": "gamescrcpy.profile-patch", "version": 1`; merges `meta`/`screen`, modifies `controls`/`vars` via `remove`/`change`/`add`, replaces `gamepad`).

### `components` and `tasks` (`component` / `task`, max 256 each)

- `components`: `{ "name", "folder" }` containing `component.ts` extending `Component` ([examples/components](../../examples/components)).
- `tasks`: `{ "name", "folder" }` containing `task.json` (`gamescrcpy.task`), `task.ts` extending `Task`, and images ([examples/tasks](../../examples/tasks)).

### `scripts` (`script`, max 8)

`{ "name", "entry", "run" }`: panel name; `.ts` (not `.d.ts`) or `.js` package path; `run` is `"session"` (per mirroring phone) or `"app"` (once per app).

### `panels` (`panel` + `script`, max 8)

Fields: `name` (panel name), `title` (<= 40 chars), `icon` (built-in name or `svg:<path>`), `where`, `script`, and `on` (required for `contextMenu` only: `["device"]`, `["profile"]`, or both).

| `where` | Location | Script |
|:--|:--|:--|
| `mirror`, `menu`, `status`, `toolbar` | Mirror side panel, picture right-click menu (must be named `menu`, max 1), status bar, toolbar button | `session` |
| `page`, `settings`, `extension`, `device`, `contextMenu`, `tray` | Main sidebar page, Settings > Extensions, extension settings dialog, device card settings, card More menu, tray menu | `app` |

SVG icons (`svg:<path>`, max 64 per extension): <= 32 KiB, root `svg`, empty or `0 0 24 24` `viewBox`, elements `path`/`circle`/`rect`/`line`/`polyline`/`polygon`/`g`/`title`/`desc` only, no `transform`/DTD/entities.

### `help` (`help`, max 16)

`{ "id", "title", "file", "locales", "icon" }`: panel name `id`, `title` <= 60 chars, English `.md` `file` (<= 1 MiB), up to 8 `locales` (`zh_CN`, `en`, ...). Opens at `help:ext/<ext-id>/<id>`. Format: [Help articles](logic.md#help-articles).

### `tours` (`tour`, max 4)

Fields: `id` (panel name), `title` (<= 60 chars), `start` (`"enabled"` default or `"manual"`), `steps` (2–12 `{ target?, title <= 60, text <= 300 }`). Targets: omitted (center card), `card`, `device` (needs device provider), `page:<panel>`, `settings:<panel>`. Replay via Details or `action:tour:ext/<ext-id>/<id>`.

### `services` (max 32)

Fields: `name` (native name; called as `<ext-id>/<name>`), `title` (<= 40), `description` (<= 500), optional `input`/`output` JSON Schemas (<= 16 KiB, <= 16 levels; `type`, `properties`, `required`, `items`, `enum`, `minimum`, `maximum`, `maxLength`, `title`, `description`), and either `script` (`"run": "app"`, requires `script`) or `node` (1 `json` param -> `json`, requires `native`). See [Services](logic.md#services-between-extensions).

### `native` (`native`)

Requires `dll` (Windows `.dll` or `{ "x64": ..., "arm64": ... }`, max 8 entries) and/or `dylib` (macOS `.dylib` or per-arch map; see [macOS](native.md#macos)). `sandbox` is `"full"` (default) or `"restricted"` ([Restricted mode](native.md#restricted-mode)). All `name`s below are native names unless noted (`title` <= 40, `description` <= 500) and must match the binary's contribution table.

| Key | Max | Fields |
|:--|:--|:--|
| `touchMethods` | 8 | `name`, `title`*, `description`, `wantsDevice` |
| `nodes` | 64 | `name`, `params` (<= 16 `{ name, type }`), `returns`, `description`, `permissions` (<= 10, declared top-level) |
| `attachments` | 32 | `name`, `title`*, `description`, `appliesTo` (component types or `*`), `params` (<= 24) |
| `behaviors` | 32 | `name`, `title`*, `description`, `slots` (<= 8 `{ name, kind: key\|axis2\|axis1, title }`), `fingers` (1–10), `frames`, `params` (<= 24) |
| `jobs` | 32 | `name` (cannot collide with `nodes`), `title`*, `params` (<= 16), `returns`, `description`, `permissions` (<= 10), `most` (1–4, default 4) |
| `deviceProviders` | 8 | `name`, `title`*, `kinds`* (`["touchBox"]`); needs `devices.provide` |
| `speechEngines` | 4 | `name`, `title`*, `say` (job), `listen` (job), `voices` (node), `dictation`, `languages` (<= 32); >= 1 of `say`/`listen` ([Speech engines](native.md#speech-engines)) |
| `inputSources` | 8 | `name` (`[a-z][a-z0-9]{0,31}`), `title`*, `slots` (1–64 `{ name: [a-zA-Z0-9_-]{1,32}, kind: button\|axis\|stick, title }`); needs `input.provide` |
| `transports` | 8 | `name`, `title`*, `icon`; needs `transport` |
| `videoSources`, `audioSources` | 8 each | `name`, `title`*, (`videoSources`: `kinds`: `phone`/`touchBox`); needs `media.capture` |
| `mediaSinks` | 8 | `name`, `title`*, `video` (`{ fps: 1–60, width/height: 16–4096 }`, <= 3840x2160), `audio` (`true`); >= 1 of `video`/`audio`; needs `media.output` |
| `recordingExporters` | 8 | `name`, `title`*, `extension`* (`[a-z0-9]+`, not `gsrec`); needs `media.output` |
| `frameTaps` | 8 | `name`, `title`*, `fps` (1–60, def 30), `size` (64–3840, def 1280); needs `media.read` |
| `aiProviders` | 4 | `name`, `title`*, `models`* (0-arg node -> `json`), `chat`* (job with `request: json` -> `json`); needs `ai.provide` ([AI providers](native.md#ai-providers)) |
| `visionEngines` | 4 | `name`, `title`*, `kinds`* (`ocr`/`detect`/`classify`), `languages` (<= 32) |

Node/job types: `boolean`, `number`, `string`, `point`, `region`, `color` (`#RRGGBB`), `duration` (ms), `json`.
Attachment/behavior `params`: `{ key, type, title* (<= 60), description, default, min, max, step, unit (<= 16), log (min > 0), options (<= 32 native names), shape (circle|rect) }` where `type` is `number`, `integer`, `toggle`, `choice`, `key`, `point` (`[x,y]`), `region` (`[x,y,r]` or `[x,y,w,h]`, pos -2..2, size 0..4), `curve` (2–16 rising `[x,y]` in 0..1), or `color`.

### `cli` (up to 16)

`{ "name", "description" (<= 200), "job", "params": [{ "name", "description", "default" }] }`. `name`: 1–32 `[a-z0-9-]` (no edge/double `-`). Job params cannot be named `json`, `hosted`, or `help`. No extension point needed. See [Command-line commands](testing.md#command-line-commands).

### `converters` (extension point `script`, up to 16)

`{ "name" (panel name), "title" (<= 40), "from", "to", "script", "extensions" (<= 8) }`. Exactly one of `from`/`to` is `profile`, `task`, or `recording`; the other is a 1–16 `[a-z0-9]` extension without dot. Imports (`to: "profile"`) accept `extensions`; exports (`from: "profile"|"task"|"recording"`) add "Export as <title>…". Runs in sandbox without phone, <= 32 MiB in/out, 30 s timeout; errors `GS-API-165`..`167`. Example: [key-list](../../examples/extensions/key-list).

### `locales` (`locale`, max 8)

`{ "locale", "name" (<= 40), "translations" (.qm <= 16 MiB), "help" (folder with >= 1 .md) }`. Cannot override built-in `en` or `zh_CN`. Compile `.ts` with `lrelease ja.ts -qm ja.qm` (`.ts` -> `locale-invalid`). Plain data, no permissions needed.

### `commands` and `presets`

Reserved in schema; not offered in 2.2.0.

## `settings`

| Field | Rule |
|:--|:--|
| `key`, `type`, `title`, `default` | Required. `type`: `choice`, `integer`, `number`, `boolean`, `text`; `title` <= 100 |
| `description`, `keywords` | Optional, <= 1000 / <= 500 chars |
| `choices` | Required for `choice`, max 64 |
| `min`, `max`, `step` | `integer`/`number` only (`step` > 0) |
| `maxLength` | `text` only, 1–4096 |
| `scope`, `live` | `scope`: `global` (default) or `device`; `live`: `true` for live-watched values |

Filter params read same-key settings; native uses `setting_get`/`settings_changed`; logic gets `settings.changed`.

## `permissions`

Only allowed when `requires` includes `script`, `component`, `task`, or `native`. Every node/job permission must appear here.

| Field | Rule |
|:--|:--|
| `name`, `reason` | Required (`reason` <= 120 chars; permission list in [Overview](README.md#permissions)) |
| `hosts` | Required for `network` (max 64): hostname + optional `:port` (default 443), `*.example.com` (subdomains only), `*` warns `permission-any-host`, ASCII `xn--…`, no IPv6 |
| `folders` | Required for `files.paths` (max 16): `{ name (panel name), purpose (<= 120) }` |
| `paths` | Legacy; grants nothing and warns `permission-paths-fixed` |
| `maxDevices` | `devices.other` only, 1–64 |

## `l10n`

Map of locale (`zh_CN` falls back to `zh`) to key -> translated string:
- `name`, `description`
- `settings.<key>.title`, `.description`, `.keywords`, `.choices.<choice>`
- `panels.<name>.title`, `help.<id>.title`, `tours.<id>.title`, `tours.<id>.<step>.title`, `tours.<id>.<step>.text` (0-indexed)
- `services.<name>.title`/`.description`, `themes.<name>.name`, `converters.<name>.title`
- `touchMethods.<name>.title`/`.description`, `nodes.<name>.description`, `attachments.<name>.*`, `behaviors.<name>.*`, `inputSources.<name>.*`, `speechEngines.<name>.title`, `transports.<name>.title`, `videoSources.<name>.title`, `audioSources.<name>.title`, `mediaSinks.<name>.title`, `recordingExporters.<name>.title`, `frameTaps.<name>.title`, `aiProviders.<name>.title`, `visionEngines.<name>.title`

## Common validation codes

| Code | Meaning |
|:--|:--|
| `missing-field`, `wrong-type`, `too-long`, `too-many`, `out-of-range`, `duplicate`, `unknown-field` (warn) | Structural / value checks |
| `id-invalid`, `id-reserved`, `version-invalid`, `range-invalid`, `gamescrcpy-missing` (warn) | Identity and version checks |
| `path-invalid`, `file-missing`, `url-invalid`, `dependency-cycle` | Paths, URLs, dependencies |
| `extension-point-unknown`, `contribution-undeclared`, `host-api-missing`, `host-api-version`, `host-api-outdated` | Extension point compatibility |
| `permission-unknown`, `permission-reason-missing`, `permission-not-allowed`, `permission-never-granted`, `permission-hosts-missing`, `permission-host-invalid`, `permission-any-host` (warn), `permission-paths-missing`, `permission-paths-fixed` (warn), `permission-field-unused` (warn) | Permission checks |
| `converter-invalid`, `locale-invalid`, `ai-provider-invalid`, `vision-engine-invalid`, `tour-target-invalid`, `speech-engine-invalid`, `service-target-invalid`, `service-schema-invalid`, `filter-compile-failed`, `filter-directive-invalid` | Contribution checks |
| `native-permission-undeclared`, `native-name-invalid`, `native-type-invalid`, `native-dll-invalid`, `panel-name-invalid`, `script-name-invalid`, `script-entry-invalid` | Native, panel, and script checks |
| `signature-invalid`, `signature-missing`, `publisher-changed`, `signing-key-mismatch` | Package signature checks ([Signing](publishing.md#signing)) |
