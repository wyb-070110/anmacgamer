# 清单参考

[English](manifest.md) · 简体中文

`manifest.json` 位于扩展文件夹根目录，为 UTF-8 JSON 对象，最大 256 KiB、最多嵌套 32 层。`gs validate` 与 anmacgamer 使用同一套规则（`--json` 输出代码、JSON 路径与修复提示）。错误阻止安装，警告（如 `unknown-field`）仅提示。

## 最小示例

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

## 顶层字段

| 字段 | 必填 | 规则 |
|:--|:--|:--|
| `schemaVersion` | 是 | 固定为 `1` |
| `id` | 是 | 2 段以上点分隔 `[a-z0-9-]`（首尾和连续不可为 `-`），最多 64 字符（`gamescrcpy.*` 保留） |
| `version` | 是 | 语义化版本 `主.次.修订`（可带预发布标签如 `1.2.0-beta.1`） |
| `gamescrcpy` | | 编写面向的 `主.次` 或 `主.次.修订`（无预发布标签）；见[为哪个版本编写](#为哪个版本编写) |
| `name`、`description` | `name` | 最多 100 / 1000 字符 |
| `authors` | | 最多 32 个，每个 <= 100 字符 |
| `license` | 是 | 许可证（推荐 SPDX 如 `MIT`、`Apache-2.0`），<= 100 字符 |
| `repository` | | `https://` 地址，<= 512 字符 |
| `authorKey` | | Minisign 公钥（`RW` 开头 56 字符）；见[打包与发布](publishing.zh-CN.md) |
| `updates` | | `{ "index": "https://…" }`；见 [`updates`](#updates) |
| `aiAssisted` | | `true` 表示借助 AI 制作 |
| `requires` | 是 | 扩展点与语义化版本范围（至少 1 项） |
| `dependencies` | | 扩展 id -> 范围或 `{ "version": 范围, "optional": true }` |
| `contributes` | | 扩展贡献项 |
| `settings`、`permissions` | | 最多 128 项设置；最多 32 项权限 |
| `l10n` | | 各语言译文 |

### 名字的写法

| 名称 | 规则 | 用途 |
|:--|:--|:--|
| 原生名 | `[a-z][a-zA-Z0-9]*`，<= 64 字符 | 原生 `name`、节点/任务参数、服务、附加/行为参数 `key` |
| 面板名 | 1–32 位 `[a-z0-9-]` | 面板、帮助/引导 `id`、脚本、转换器、`files.paths` 文件夹名 |
| 方案名 | 1–64 位 `[a-zA-Z0-9_-]` | 方案、组件（<= 48）、任务 |
| 设置键 | `[a-zA-Z][a-zA-Z0-9]*`，<= 64 字符 | `settings[].key` |
| 包内路径 | 相对路径、`/` 分隔、不出目录、无 `\ : * ? " < > \|`，<= 260 字符（单段 <= 255，不以 `.`/空格结尾，非 Windows 保留名 `con`/`nul`/`com1` 等） | `file`、`entry`、`folder`、`dll`、`dylib` |

点开头文件/目录（`.git`、`.build`）不检查也不打包。

### `requires` 与版本范围

范围语法：`^1.0`（`>=1.0 <2.0`，`0.x` 锁次版本）、`~1.2`（`>=1.2 <1.3`）、空格分隔比较（`>=1.0.0 <2.0.0`）、精确（`1.2.3` / `=1.2.3`）、`*`。预发布版仅匹配同版本预发布范围。

次版本增量兼容；主版本不兼容变更；单字段收紧（如 `files.paths` 的 `paths`）仅警告不升主版本。

| 扩展点 | 2.0.0 | 2.1.0 | 2.2.0 | 说明 |
|:--|:--|:--|:--|:--|
| `filter`、`theme`、`profile` | 1.0 | 1.0 | 1.0 | — |
| `script`、`component`、`task` | 1.0 | 1.1 | 1.1 | 2.1.0：`net`、`files`、`speech`、`events`、`media`、`ext.provide`/`ext.call`、转换器、新 `ui.*` |
| `panel` | 1.0 | 1.1 | 1.1 | 2.1.0：`device`、`settings`、`extension`、`toolbar`、`contextMenu`、`tray`、`on` |
| `native` | 1.0 | 2.0 | 2.1 | 2.1.0：接口 2（`gs_ext_main`、长任务、提供者、媒体、AI、受限模式）；2.2.0：AI `xhigh`/`max`、`topEffortsThink`、`millionContext`（[原生扩展指南](native.zh-CN.md)） |
| `help`、`tour`、`locale` | — | 1.0 | 1.0 | 2.1 新增 |
| `command`、`preset` | — | — | — | 保留，暂未提供 |

兼容性结果（`gs validate`、`gs pack` 与卡片）：
- `host-api-missing`：程序无此扩展点（“需要更新版的 anmacgamer”）。
- `host-api-version`：要求版本过高（“需要更新版的 anmacgamer”）。
- `host-api-outdated`：程序版本高于范围上限（“为旧版 anmacgamer 制作”；仅下界如 `>=1.0` 不会触发）。

### 为哪个版本编写

可选 `"gamescrcpy": "2.1"`（`主.次` 或 `主.次.修订`，无预发布；非法报 `version-invalid`）。仅用于界面展示，兼容性始终由 `requires` 决定。缺失时按 `requires` 推算；`gs validate`/`gs pack` 报 `gamescrcpy-missing` 警告。

### `dependencies`

映射 `<扩展 id>` 到版本范围或 `{ "version": 范围, "optional": true }`。必需依赖缺失/关闭/不兼容时扩展不启用。决定加载顺序与跨扩展服务/事件权限（[逻辑扩展指南](logic.zh-CN.md)）。依赖自身报 `dependency-cycle`。

### `updates`

`{ "index": "https://…" }`：带主机名、无用户名的 `https://` 可打印 ASCII，<= 2048 字符（否则 `url-invalid`）。仅手动检查或每日启动检查（默认关）时请求，不发送扩展 `id`。格式见[更新列表](publishing.zh-CN.md#更新列表)。

## `contributes`

每类贡献均须在 `requires` 声明对应扩展点（否则 `contribution-undeclared`），同类内 `name`/`id` 唯一。

### `themes`（`theme`，最多 32）

`{ "name": "Slate", "dark": { "bgBase": "#0E1116" }, "light": { "bgBase": "#E6EAF1" } }`。`name` <= 64；令牌 `[a-z][a-zA-Z0-9]*`；颜色 `#RRGGBB` 或 `#RRGGBBAA`。示例：[slate-theme](../../examples/extensions/slate-theme)。

### `filters`（`filter`，最多 32）

`{ "name": "Scanlines", "file": "shaders/scanlines.hlsl" }`（`.hlsl` <= 1 MiB）。指令：
- `//! PARAM <名> <float|int|bool> <默认> [<最小> <最大> [<步长>]]`（最多 64；读同名 `settings`）
- `//! PASS <名> [scale=<比例>] [format=rgba8|rgba16f]`（1–16；`float4 <名>(float2 uv)`，`0 < scale <= 4`）
- `//! TEXTURE <名> <文件.png>`（最多 8）

内置变量：`Original`、`Source`、`Linear`、`Point`、`SourceSize`、`OriginalSize`、`OutputSize`、`Frame`（`Gs*` 保留）。示例：[crt-scanlines](../../examples/extensions/crt-scanlines)。

### `profiles` 与 `profilePatches`（`profile`，各最多 256）

- `profiles`：`{ "name", "file" }`（方案名、`gamescrcpy.profile` JSON <= 1 MiB）。
- `profilePatches`：`{ "target": "<扩展 id>/<方案名>", "file" }`（`"format": "gamescrcpy.profile-patch", "version": 1`；合并 `meta`/`screen`，用 `remove`/`change`/`add` 改 `controls`/`vars`，替换 `gamepad`）。

### `components` 与 `tasks`（`component` / `task`，各最多 256）

- `components`：`{ "name", "folder" }`，含继承 `Component` 的 `component.ts`（[examples/components](../../examples/components)）。
- `tasks`：`{ "name", "folder" }`，含 `task.json`（`gamescrcpy.task`）、继承 `Task` 的 `task.ts` 及图片（[examples/tasks](../../examples/tasks)）。

### `scripts`（`script`，最多 8）

`{ "name", "entry", "run" }`：面板名；`.ts`（非 `.d.ts`）或 `.js` 包内路径；`run` 为 `"session"`（每台投屏手机）或 `"app"`（整个程序一份）。

### `panels`（`panel` + `script`，最多 8）

字段：`name`（面板名）、`title`（<= 40）、`icon`（内置名或 `svg:<路径>`）、`where`、`script`，以及 `on`（仅 `contextMenu` 必填：`["device"]`、`["profile"]` 或两者）。

| `where` | 位置 | 脚本 |
|:--|:--|:--|
| `mirror`、`menu`、`status`、`toolbar` | 投屏侧边栏、画面右键菜单（名须为 `menu`，最多 1 个）、状态区、工具栏按钮 | `session` |
| `page`、`settings`、`extension`、`device`、`contextMenu`、`tray` | 主窗口左栏页、设置 > 扩展、扩展设置窗、设备卡片设置、卡片“更多”菜单、托盘菜单 | `app` |

SVG 图标（`svg:<路径>`，每扩展最多 64 个）：<= 32 KiB，根元素 `svg`，空或 `0 0 24 24` 的 `viewBox`，仅含 `path`/`circle`/`rect`/`line`/`polyline`/`polygon`/`g`/`title`/`desc`，禁止 `transform`/DTD/实体。

### `help`（`help`，最多 16）

`{ "id", "title", "file", "locales", "icon" }`：面板名 `id`、`title` <= 60、英文 `.md` `file`（<= 1 MiB）、最多 8 个 `locales`（`zh_CN`、`en` 等）。地址 `help:ext/<扩展 id>/<id>`，写法见[帮助文章](logic.zh-CN.md#帮助文章)。

### `tours`（`tour`，最多 4）

字段：`id`（面板名）、`title`（<= 60）、`start`（默认 `"enabled"` 或 `"manual"`）、`steps`（2–12 步 `{ target?, title <= 60, text <= 300 }`）。目标：省略（居中卡片）、`card`、`device`（需设备提供者）、`page:<面板>`、`settings:<面板>`。详情页或 `action:tour:ext/<扩展 id>/<id>` 可重放。

### `services`（最多 32）

字段：`name`（原生名，调用为 `<扩展 id>/<name>`）、`title`（<= 40）、`description`（<= 500）、可选 `input`/`output` JSON Schema（<= 16 KiB、<= 16 层；`type`、`properties`、`required`、`items`、`enum`、`minimum`、`maximum`、`maxLength`、`title`、`description`），以及 `script`（`"run": "app"`，需 `script`）或 `node`（单 `json` 参数返回 `json`，需 `native`）。见[扩展之间的服务](logic.zh-CN.md#扩展之间的服务)。

### `native`（`native`）

需 `dll`（Windows `.dll` 或 `{ "x64": ..., "arm64": ... }`，最多 8 项）和/或 `dylib`（macOS `.dylib` 或按架构映射；见 [macOS](native.zh-CN.md#macos)）。`sandbox` 为 `"full"`（默认）或 `"restricted"`（[受限模式](native.zh-CN.md#受限模式)）。下表除注明外 `name` 为原生名（`title` <= 40、`description` <= 500），须与二进制贡献表一致。

| 键 | 上限 | 字段 |
|:--|:--|:--|
| `touchMethods` | 8 | `name`、`title`*、`description`、`wantsDevice` |
| `nodes` | 64 | `name`、`params`（<= 16 `{ name, type }`）、`returns`、`description`、`permissions`（<= 10，须在顶层声明） |
| `attachments` | 32 | `name`、`title`*、`description`、`appliesTo`（组件类型或 `*`）、`params`（<= 24） |
| `behaviors` | 32 | `name`、`title`*、`description`、`slots`（<= 8 `{ name, kind: key\|axis2\|axis1, title }`）、`fingers`（1–10）、`frames`、`params`（<= 24） |
| `jobs` | 32 | `name`（不与 `nodes` 重名）、`title`*、`params`（<= 16）、`returns`、`description`、`permissions`（<= 10）、`most`（1–4，默认 4） |
| `deviceProviders` | 8 | `name`、`title`*、`kinds`*（`["touchBox"]`）；需 `devices.provide` |
| `speechEngines` | 4 | `name`、`title`*、`say`（任务）、`listen`（任务）、`voices`（节点）、`dictation`、`languages`（<= 32）；至少 `say`/`listen` 之一（[语音引擎](native.zh-CN.md#语音引擎)） |
| `inputSources` | 8 | `name`（`[a-z][a-z0-9]{0,31}`）、`title`*、`slots`（1–64 `{ name: [a-zA-Z0-9_-]{1,32}, kind: button\|axis\|stick, title }`）；需 `input.provide` |
| `transports` | 8 | `name`、`title`*、`icon`；需 `transport` |
| `videoSources`、`audioSources` | 各 8 | `name`、`title`*（`videoSources` 含 `kinds`：`phone`/`touchBox`）；需 `media.capture` |
| `mediaSinks` | 8 | `name`、`title`*、`video`（`{ fps: 1–60, width/height: 16–4096 }`，<= 3840x2160）、`audio`（`true`）；至少其一；需 `media.output` |
| `recordingExporters` | 8 | `name`、`title`*、`extension`*（`[a-z0-9]+`，非 `gsrec`）；需 `media.output` |
| `frameTaps` | 8 | `name`、`title`*、`fps`（1–60，默认 30）、`size`（64–3840，默认 1280）；需 `media.read` |
| `aiProviders` | 4 | `name`、`title`*、`models`*（无参节点返回 `json`）、`chat`*（单参 `request: json` 任务返回 `json`）；需 `ai.provide`（[AI 服务提供者](native.zh-CN.md#ai-服务提供者)） |
| `visionEngines` | 4 | `name`、`title`*、`kinds`*（`ocr`/`detect`/`classify`）、`languages`（<= 32） |

节点/任务类型：`boolean`、`number`、`string`、`point`、`region`、`color`（`#RRGGBB`）、`duration`（毫秒）、`json`。
附加/行为 `params`：`{ key, type, title* (<= 60), description, default, min, max, step, unit (<= 16), log (min > 0), options (<= 32 原生名), shape (circle|rect) }`，`type` 可为 `number`、`integer`、`toggle`、`choice`、`key`、`point`（`[x,y]`）、`region`（`[x,y,r]` 或 `[x,y,w,h]`，位置 -2..2、尺寸 0..4）、`curve`（2–16 个递增 `[x,y]`，范围 0..1）、`color`。

### `cli`（最多 16 个）

`{ "name", "description" (<= 200), "job", "params": [{ "name", "description", "default" }] }`。`name` 为 1–32 位 `[a-z0-9-]`（首尾与连续不可为 `-`）。任务参数不可命名为 `json`、`hosted`、`help`。无需扩展点。见[命令行子命令](testing.zh-CN.md#命令行子命令)。

### `converters`（扩展点 `script`，最多 16 个）

`{ "name" (面板名), "title" (<= 40), "from", "to", "script", "extensions" (<= 8) }`。`from`/`to` 恰好其一为 `profile`、`task` 或 `recording`，另一为 1–16 位 `[a-z0-9]` 扩展名（无点）。导入（`to: "profile"`）认 `extensions`，导出（`from: "profile"|"task"|"recording"`）加菜单项“导出为 <title>…”。沙箱内运行（无手机），进出 <= 32 MiB，30 秒超时，错误 `GS-API-165`..`167`。示例：[key-list](../../examples/extensions/key-list)。

### `locales`（`locale`，最多 8）

`{ "locale", "name" (<= 40), "translations" (.qm <= 16 MiB), "help" (含 >= 1 个 .md 的目录) }`。不可覆盖内置 `en`/`zh_CN`。用 `lrelease ja.ts -qm ja.qm` 编译（写 `.ts` 报 `locale-invalid`）。纯数据，无需权限。

### `commands` 与 `presets`

格式保留，2.2.0 暂不提供。

## `settings`

| 字段 | 规则 |
|:--|:--|
| `key`、`type`、`title`、`default` | 必填。`type`：`choice`、`integer`、`number`、`boolean`、`text`；`title` <= 100 |
| `description`、`keywords` | 可选，<= 1000 / <= 500 字符 |
| `choices` | `choice` 必填，最多 64 项 |
| `min`、`max`、`step` | 仅 `integer`/`number`（`step` > 0） |
| `maxLength` | 仅 `text`，1–4096 |
| `scope`、`live` | `scope`：`global`（默认）或 `device`；`live`：`true` 表示实时监听 |

滤镜读同名设置；原生用 `setting_get`/`settings_changed`；逻辑收 `settings.changed`。

## `permissions`

仅 `requires` 含 `script`、`component`、`task` 或 `native` 时可用。节点/任务声明的权限须同步列于此处。

| 字段 | 规则 |
|:--|:--|
| `name`、`reason` | 必填（`reason` <= 120 字符；权限列表见[概览](README.zh-CN.md#权限)） |
| `hosts` | `network` 必填（最多 64）：主机名+可选 `:端口`（默认 443），`*.example.com`（仅子域），`*` 报 `permission-any-host` 警告，ASCII `xn--…`，不支持 IPv6 |
| `folders` | `files.paths` 必填（最多 16）：`{ name (面板名), purpose (<= 120) }` |
| `paths` | 旧字段，不授权限并报 `permission-paths-fixed` 警告 |
| `maxDevices` | 仅 `devices.other`，1–64 |

## `l10n`

语言（`zh_CN` 回退 `zh`）到键 -> 译文的映射：
- `name`、`description`
- `settings.<key>.title`、`.description`、`.keywords`、`.choices.<选项>`
- `panels.<name>.title`、`help.<id>.title`、`tours.<id>.title`、`tours.<id>.<步>.title`、`tours.<id>.<步>.text`（从 0 起）
- `services.<name>.title`/`.description`、`themes.<name>.name`、`converters.<name>.title`
- `touchMethods.<name>.title`/`.description`、`nodes.<name>.description`、`attachments.<name>.*`、`behaviors.<name>.*`、`inputSources.<name>.*`、`speechEngines.<name>.title`、`transports.<name>.title`、`videoSources.<name>.title`、`audioSources.<name>.title`、`mediaSinks.<name>.title`、`recordingExporters.<name>.title`、`frameTaps.<name>.title`、`aiProviders.<name>.title`、`visionEngines.<name>.title`

## 常见检查代码

| 代码 | 含义 |
|:--|:--|
| `missing-field`、`wrong-type`、`too-long`、`too-many`、`out-of-range`、`duplicate`、`unknown-field`（警告） | 结构与值检查 |
| `id-invalid`、`id-reserved`、`version-invalid`、`range-invalid`、`gamescrcpy-missing`（警告） | 标识与版本检查 |
| `path-invalid`、`file-missing`、`url-invalid`、`dependency-cycle` | 路径、URL 与依赖 |
| `extension-point-unknown`、`contribution-undeclared`、`host-api-missing`、`host-api-version`、`host-api-outdated` | 扩展点兼容性 |
| `permission-unknown`、`permission-reason-missing`、`permission-not-allowed`、`permission-never-granted`、`permission-hosts-missing`、`permission-host-invalid`、`permission-any-host`（警告）、`permission-paths-missing`、`permission-paths-fixed`（警告）、`permission-field-unused`（警告） | 权限检查 |
| `converter-invalid`、`locale-invalid`、`ai-provider-invalid`、`vision-engine-invalid`、`tour-target-invalid`、`speech-engine-invalid`、`service-target-invalid`、`service-schema-invalid`、`filter-compile-failed`、`filter-directive-invalid` | 贡献项检查 |
| `native-permission-undeclared`、`native-name-invalid`、`native-type-invalid`、`native-dll-invalid`、`panel-name-invalid`、`script-name-invalid`、`script-entry-invalid` | 原生、面板与脚本检查 |
| `signature-invalid`、`signature-missing`、`publisher-changed`、`signing-key-mismatch` | 包签名检查（[签名](publishing.zh-CN.md#签名)） |
