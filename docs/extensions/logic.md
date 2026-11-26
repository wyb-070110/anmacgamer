# Logic guide

English · [简体中文](logic.zh-CN.md)

Extension logic is written in TypeScript (or JavaScript) against the same script API as profile logic ([sdk/docs](../../sdk/docs/README.md)). It runs in the low-privilege `gs-scripthost.exe` process; all OS, device, network, and file access goes through anmacgamer and is permission-checked.

## Scripts

```json
"requires": { "script": "^1.1" },
"contributes": {
  "scripts": [
    { "name": "main", "entry": "src/main.ts", "run": "app" },
    { "name": "phone", "entry": "src/phone.ts", "run": "session" }
  ]
}
```

| | App script (`"run": "app"`) | Session script (`"run": "session"`) |
|:--|:--|:--|
| Instances | One while extension is enabled | One per mirroring phone |
| Allowed | Main-window UI (`page`, `settings`, menus, `tray`), services, MCP tools, `net.*`, `files.*`, `speech.*`, `hardware.*`, app events | Mirror-window UI (`mirror`, `menu`, `status`, `toolbar`), phone `touch.*`, `vision.*`, keymap, clipboard, `net.*`, `files.*` |
| Disallowed | Phone APIs (`touch.*`, `vision.*`, `ui.toast` -> `GS-API-073` / `GS-API-001`) | Main-window UI |

Rules:
- Missing permissions throw `GS-API-072`. User's own logic has no permission restrictions.
- Supports relative `import ... from "./util.ts"`. Types are erased (not compiled), so `enum`, code-bearing `namespace`, constructor parameter properties, decorators, `import x = …`, and `export =` are disallowed.
- Resources opened inside a task (event handler, `every`, etc.) close when the task ends; top-level resources close when logic stops (listeners, `net.listen`, speech listeners, hardware handles/claims).
- `store.get` / `store.set` persists up to 1 MiB JSON per script (per phone for session scripts). Scripts cannot read manifest `settings` directly; they only receive `settings.changed` with changed keys.
- `log.info` etc. write to program logs (`logic <name> on <phone>: …`). App-script crashes restart after 1 s; 3 stops in 1 minute disable the extension's logic.
- Add `tsconfig.json` (see [examples/components](../../examples/components)), run `gs types <folder>` for editor types, and `gs check <folder>` (expects `src/main.ts` or `src/main.js`).

## Panels and places in the interface

Scripts emit a declarative UI tree; anmacgamer renders it with native controls and sends back user events. Declare panels in `contributes.panels` with `requires`: `panel` and `script` ([manifest reference](manifest.md)).

```ts
const page = ui.panel("laps");
let laps: number[] = [];
function draw(): void {
    page.render({
        type: "column",
        children: [
            { type: "text", text: `${laps.length} laps`, style: "heading" },
            { type: "table", columns: ["Lap", "Time"], rows: laps.map((ms, i) => [i + 1, `${ms} ms`]) },
            { type: "button", id: "clear", title: "Clear", style: "danger", enabled: laps.length > 0 },
        ],
    });
}
page.on("click", ({ id }) => { if (id === "clear") { laps = []; draw(); } });
draw();
```

- `render(tree)` updates UI (`render(null)` clears). Limits: 256 KiB, 2000 nodes, 16 levels, max 30 draws/s. Errors: `GS-API-075` (invalid tree), `GS-API-076` (undeclared panel).
- Nodes: `column`, `row`, `group`, `text` (max 4000 chars; `format: "markdown"` supports bold/italic/code/lists, no links/images), `button`, `field`, `number`, `toggle`, `choice`, `slider`, `list`, `table` (max 16 cols, 200 rows), `chart`, `canvas`, `image` (PNG/SVG <= 2 MiB in package), `progress`, `separator`, `deviceItem`, `steps`, `qr`, `keyValue`. Interactive nodes need `id`. Tree text does not use `l10n`.
- Events: `click`, `change` (with `value`), `submit`, `select`, `open`, `close`. Handler receives `{ id, value, signal }`.

| `where` | Script API |
|:--|:--|
| `mirror`, `page`, `settings`, `extension`, `device` | `ui.panel(name).render()` / `.on()` (`device` `open`/`close` passes device `id`; `settings` fires `open`/`close` while section is open) |
| `menu` | `ui.menu(items)` and `ui.menu.on("select", …)` |
| `status` | `ui.status(name).render(tree)` (max 64 nodes, 100 chars) |
| `toolbar` | `ui.toolbar(name).set({ title, icon, checked, enabled })` (`set(null)` resets), `.on("click", …)`; same-name `ui.panel` pops up below button |
| `contextMenu` | `ui.contextMenu(name).set(items)` (max 16), `.on("choose", ({ id, value, target }) => …)` (`target`: `{ kind: "device" \| "profile", id, name }`) |
| `tray` | `ui.tray(name).set(items)` (max 16), `.on("choose", …)` |

Menu items: `{ id, title, icon, checked, enabled }` (`checked` toggles `value`). Icons accept built-in names or `svg:<package path>` on panels, toolbar buttons, and `deviceItem`.

## Program events

```ts
events.on("profile.applied", ({ value }) => log.info("profile applied", JSON.stringify(value)));
```

`events.on(topic, handler, options)` runs a task per event with `{ topic, value, signal }`. See topics in [Native guide](native.md#program-events) and [sdk/docs/events.md](../../sdk/docs/events.md).
- User logic receives all events. Extension logic always receives `extension.enabled`, `extension.disabled`, and `settings.changed` (own settings, language, theme).
- Session scripts receive their own phone's events. Other phones (and phone events in app scripts/tasks) require `events.app` (silently omitted if absent).
- Custom native events from `emit_event`: `<topic>` for own extension, `"<extension id>:<topic>"` for extensions in `dependencies`.
- Rate limit: first 100 events/s per topic deliver immediately; remaining events in that second coalesce to the last.

## Services between extensions

Declare in `contributes.services` and register in an app script:

```ts
ext.provide<{ track: string }>("best", ({ track }, { caller }) => best.get(track) ?? null);
```

Callers list the provider in `dependencies` (optional counts) and call:

```ts
const ms = await ext.call<number | null>("alice.lap-timer/best", { track: "monza" });
```

- Name format: `<extension id>/<service>`. Extension logic may only call own or declared dependency services (`GS-API-147`); user logic may call any enabled extension.
- Handler receives `(args, { caller, permissions, signal })` (`caller === ""` for user logic; `signal` aborts on cancel or 10 s timeout). Return value is result (`undefined` -> `null`); throwing fails the call.
- Permissions are the **intersection** of provider and caller (missing permission -> `GS-API-072`).
- JSON payloads <= 252 KiB validated against manifest `input`/`output` (`type`, `properties`, `required`, `items`, `enum`, `minimum`, `maximum`, `maxLength`, `title`, `description`). Max 256 in-flight calls per logic.
- Errors: `GS-API-145` (undeclared for script), `GS-API-146` (unavailable or not provided within 10 s), `GS-API-147` (missing dependency), `GS-API-148` (schema/JSON/size), `GS-API-149` (handler threw, > 10 s, or > 256 waiting). Native nodes can also back services ([Native guide](native.md#services)).

## Incoming local connections

`net.listen(port, handler, { origins })` binds HTTP on `127.0.0.1` (requires `network.listen`; toggleable in extension Details).

```ts
const server = await net.listen(8765, async (req) => {
    if (req.method !== "POST") return { status: 405, headers: { allow: "POST" }, json: { error: "send POST" } };
    return { json: { ok: true, body: req.json() } };
});
```

- Ports 1024–65535; max 4 servers per extension. Errors: `GS-API-100` (no permission/disabled), `GS-API-101` (bad port/origin), `GS-API-102` (port in use or > 4 servers).
- Request: `{ method, path, query, headers, text, bytes, json() }` (decoded `path`, lowercase headers). Response: `undefined` (204), string (200 text/plain), or `{ status, headers, text \| json \| bytes }`. Throw -> 500; > 10 s -> 504.
- Security: `127.0.0.1` only; `Host` must be `127.0.0.1:<port>` or `localhost:<port>`; `Origin` allowed only for same server or `origins` (max 16 `https://...`, preflight handled); `Origin: null` rejected. Limits: 16 KiB headers, 1 MiB body, 16 concurrent connections, connection closes per response, HTTP only (no WebSocket). Use `POST` for mutations.
- Turning off the switch closes servers (`server.isOpen` becomes `false`); re-enabling does not reopen them. Example: [deck-bridge](../../examples/extensions/deck-bridge).

## The network

```ts
const reply = await net.fetch("https://api.github.com/repos/wyb-070110/anmacgamer/releases/latest", {
    headers: { accept: "application/vnd.github+json" },
});
```

- `net.fetch(url, { method, headers, body, timeoutMs })`: HTTPS only, resolves to `{ status, url, headers, text, bytes, json() }` (lowercase headers, final `url`). Extension logic needs `network` and can only reach manifest `hosts` ([manifest reference](manifest.md#permissions)), max 5 redirects to allowed hosts.
- Limits: 1 MiB request body, 8 MiB response, 8 concurrent requests, `timeoutMs` 1000–60000 (default 15000). No cookie jar or cache; `host`, `content-length`, `proxy-*`, `sec-*` headers blocked; invalid TLS certs fail.
- `net.socket(url, { protocols })`: `wss://` WebSocket (`NetSocket`: `send` <= 1 MiB, `close`, `onMessage`, `onClose`, `isOpen`, `protocol`), same `hosts` rules, max 4 open per logic.
- Errors: `GS-API-155` (permission/host/redirect), `GS-API-156` (scheme/method/header/body/timeout), `GS-API-157` (size/count/backlog), `GS-API-158` (network/TLS/redirects/timeout), `GS-API-159` (closed socket).

## Files

```ts
const file = await files.pick({ filters: [{ name: "Key lists", extensions: ["keys"] }] });
const notes = await files.folder("notes");
await notes.write("today.txt", "Bring the map.");
```

- `files.pick({ save, filters, suggestedName })` (requires `files.pick`): opens OS file dialog, returns handle (`read()`, `text()`, `write()`, `close()`, <= 64 MiB) or `null`.
- `files.folder(name)` (requires `files.paths` with `name` in manifest `folders`; user logic allows 1–32 `[a-z0-9-]`): prompts once with `purpose` and remembers folder. Methods: `list(subfolder)` (<= 4096 entries), `read(path)`, `text(path)`, `write(path, data)` (atomic, creates dirs), `remove(path)`, `close()`. `/` paths only; `..`, drives, and escaping links rejected.
- Limits: 64 open handles, 8 pending user prompts per logic.
- Errors: `GS-API-160` (permission/undeclared folder), `GS-API-161` (bad options/name/path), `GS-API-162` (OS error), `GS-API-163` (size/count), `GS-API-164` (closed handle/cancelled folder dialog). Example: [notes-panel](../../examples/extensions/notes-panel).

## Reading aloud and voice commands

```ts
await speech.say("Ready", { rate: 1.1 });
const listener = await speech.listen(["open map", "close map"], async ({ phrase, confidence }) => {
    log.info("heard", phrase, confidence);
}, { confidence: 0.7 });
```

- `speech.say(text, { engine, voice, rate, volume, interrupt })` (no permission): 1–1000 chars, returns `true` if finished or `false` if interrupted/`speech.stop()`/cancelled. Queued per engine unless `interrupt: true`.
- `speech.listen(commands, handler, { engine, language, confidence })` (requires `speech.listen`, else `GS-API-113`): 1–256 commands of 1–64 chars (`null` if engine supports free dictation), default `confidence` 0.6, handler receives `{ phrase, text, confidence }`, `listener.isOpen`.
- `speech.engines()` lists `system` and `<extension id>/<name>`; `speech.voices({ engine })` lists voices; omitted options use Settings > Audio.
- Errors: `GS-API-110` (engine/capability/language/voice), `GS-API-111` (args/limits), `GS-API-112` (engine failure), `GS-API-113` (permission). Example: [voice-commands](../../examples/extensions/voice-commands).

## Hardware and home-made devices

- `hardware.request({ kind, filters })`, `hardware.list()`, `hardware.open(id, serialOptions)` (exclusive open), `hardware.forget(id)`. Requires `hardware` (`GS-API-080`); unselected devices fail with `GS-API-081`.
- `hardware.claim(resource, { wait, onWanted })` / `hardware.release(resource)` coordinate hardware with other extensions/sessions (`hardware.open` auto-claims).
- `input.source(name)` (`hardware` permission): `press(key)`, `release(key)`, `axis(key, x, y)` bind as `Ext.<extension id>.<name>.<key>` (`Ext.mine.<name>.<key>` for user logic). App scripts target the front mirror window.

## Native nodes and jobs

Exposed under `ext.<extension>` (each id segment in lowerCamelCase):

```ts
const version = ext.alice.board.version();
const job = ext.alice.board.flash({ port: "COM9" });
job.onProgress((fraction, text) => log.info(Math.round(fraction * 100), text));
job.onOutput((value) => log.info("found", JSON.stringify(value)));
const result = await job.result;
```

Nodes return within 100 ms. Jobs return `ExtensionJob` (`result`, `onProgress`, `onLog`, `onOutput`, `follow(progress, done, failed)`, `cancel()`) and auto-cancel on logic stop. Error codes: [Native guide](native.md#errors-that-logic-sees).

## Frames of the phone's picture

Session scripts only (`media.read` permission): `media.frames(handler, { fps, size, region })` (`fps` 0.1–10, default 2; `size` long side 64–1920, default 640) passes `{ image, time }`. Errors: `GS-API-073` (app script), `GS-API-120` (options), `GS-API-121` (no picture), `GS-API-122` (> 4 subscriptions).

## Recognition

```ts
const lines = await vision.recognize("system", "ocr", { x: 0, y: 0.6, width: 0.4, height: 0.3 }, { language: "zh-Hans" });
```

- `vision.recognize(engine, kind, region, params)` (session scripts only, requires `device.screen`): `kind` is `ocr`, `detect`, or `classify`. Uses next frame (or current frame after 0.25 s; `null` if no picture in 2 s). Results have normalized center `x, y`, `width, height`, and `confidence` (0–1). `region` may be a `res/resources.json` name or omitted for full frame.
- `vision.engines()` lists `system` (params: `language`, `words: true`) and `<extension id>/<name>`. `vision.engine(id)` offers `.read()`, `.detect()`, `.classify()`. Extension engines time out after 5 s (1 frame at a time).
- Errors: `GS-API-170` (missing/disabled engine), `GS-API-171` (unsupported kind/language), `GS-API-172` (bad args), `GS-API-173` (engine error), `GS-API-174` (> 5 s, busy, or 16 queued). Example: [read-aloud](../../examples/extensions/read-aloud); custom engine: [Native guide](native.md#recognition-engines).

## MCP tools and AI

- App scripts: `mcp.tool(name, { description, input }, handler)` registers `ext_<extension id>_<name>` (dots/hyphens -> `_`), user-confirmed per call, 60 s timeout, max 32 tools per extension.
- `ai.ask(question, { picture, timeoutMs })`: requires `ai` permission and Settings > AI "Let logic call AI", max 10 calls/min, no `picture` in app scripts. Unavailable extension AI provider throws `GS-API-150` ([Native guide](native.md#ai-providers)).

## Converters

Declared in `contributes.converters` ([manifest reference](manifest.md#converters-extension-point-script-up-to-16)); script exports `convert` (or default) taking `ConvertInput` (`from`, `to`, `text`, `json()`, `signal` — 30 s timeout) and returning `ConvertOutput` (`name`, `json`, `text`). Limits: 32 MiB in / 32 MiB out. Errors: `GS-API-165` (no export), `GS-API-166` (unwritable result), `GS-API-167` (> 32 MiB), `GS-API-168` (unreadable input). Example: [key-list](../../examples/extensions/key-list).

## Guided tours

Up to 4 tours in `contributes.tours` (extension point `tour`; [manifest reference](manifest.md)), 2–12 steps, titles <= 60 chars, text <= 300 chars. `start: "enabled"` plays once on first enable (unless tours are off). Missing targets are skipped (render `page`/`settings` panels first). Replay in Details or via `action:tour:ext/<extension id>/<tour id>`. `l10n` keys: `tours.<id>.title`, `tours.<id>.<step>.title`, `tours.<id>.<step>.text` (0-indexed).

## Help articles

Up to 16 articles in `contributes.help` (extension point `help`), indexed by help search and AI `read_help`. `file` is English Markdown; `locales` maps other languages. Supports `#` headings, lists (`1.` bold lead becomes step title), shortcut tables (`<kbd>` in col 1), fenced code blocks, `> [!TIP]`/`[!NOTE]`/`[!WARNING]` callouts (trailing single link becomes button), relative images `![caption](images/pic.png)` (no `:` or `figure:`), and links: `help:<article>`, `help:ext/<extension id>/<id>`, `action:tour:ext/<extension id>/<tour id>`, `https://`. Example: [tour-and-help](../../examples/extensions/tour-and-help).

## Common error codes

| Code | Meaning |
|:--|:--|
| `GS-API-001`, `GS-API-073` | No phone connected (app script called phone-only API) |
| `GS-API-072` | Missing or denied permission (or service caller lacks it) |
| `GS-API-075`, `GS-API-076` | Invalid UI tree; undeclared panel for script |
| `GS-API-080`–`GS-API-085` | Hardware: permission, unallowed device, missing, open failed, bad options |
| `GS-API-100`–`GS-API-102` | Local server: permission/disabled, bad port/origin, port busy or limit |
| `GS-API-110`–`GS-API-113` | Speech: engine, args/limits, engine error, permission |
| `GS-API-120`–`GS-API-122` | Frames: options, no picture, too many subscriptions |
| `GS-API-145`–`GS-API-149` | Services: undeclared, unavailable, no dependency, schema/size, failure/timeout |
| `GS-API-150` | Selected AI provider extension is unavailable |
| `GS-API-155`–`GS-API-159` | Network: permission/host, bad args, size/count, unreachable/TLS/timeout, closed socket |
| `GS-API-160`–`GS-API-164` | Files: permission/folder, bad args/path, OS error, size/count, closed/cancelled |
| `GS-API-165`–`GS-API-168` | Converters: no export, unwritable, > 32 MiB, unreadable |
| `GS-API-170`–`GS-API-174` | Recognition: engine, kind/lang, args, failure, timeout/busy |
