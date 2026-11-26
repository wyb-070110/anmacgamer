# 逻辑扩展指南

[English](logic.md) · 简体中文

扩展逻辑用 TypeScript（或 JavaScript）编写，使用与方案逻辑相同的脚本接口（[sdk/docs](../../sdk/docs/README.md)）。它运行在低权限 `gs-scripthost.exe` 进程中，所有系统、设备、网络与文件操作都经 anmacgamer 校验权限后代办。

## 脚本

```json
"requires": { "script": "^1.1" },
"contributes": {
  "scripts": [
    { "name": "main", "entry": "src/main.ts", "run": "app" },
    { "name": "phone", "entry": "src/phone.ts", "run": "session" }
  ]
}
```

| | 应用脚本（`"run": "app"`） | 会话脚本（`"run": "session"`） |
|:--|:--|:--|
| 实例数 | 扩展启用期间 1 份 | 每台投屏手机各 1 份 |
| 可用 | 主窗口界面（`page`、`settings`、菜单、`tray`）、服务、MCP 工具、`net.*`、`files.*`、`speech.*`、`hardware.*`、程序事件 | 投屏窗口界面（`mirror`、`menu`、`status`、`toolbar`）、本机 `touch.*`、`vision.*`、键位、剪贴板、`net.*`、`files.*` |
| 不可用 | 需手机的接口（`touch.*`、`vision.*`、`ui.toast` -> `GS-API-073` / `GS-API-001`） | 主窗口界面 |

规则：
- 缺少权限抛 `GS-API-072`；用户自有逻辑无权限限制。
- 支持相对路径 `import ... from "./util.ts"`。仅擦除类型不编译，禁止 `enum`、含代码 `namespace`、构造函数参数属性、装饰器、`import x = …` 与 `export =`。
- 任务内（事件处理、`every` 等）打开的资源随任务结束关闭；顶层资源随逻辑停止关闭（监听、`net.listen`、语音监听、硬件句柄与占用）。
- `store.get` / `store.set` 持久化最多 1 MiB JSON（会话脚本按手机隔离）。脚本不能直接读清单 `settings`，只能收 `settings.changed`（含变更键）。
- `log.info` 等写入程序日志（`logic <名字> on <手机>: …`）。应用脚本崩溃 1 秒后重启，1 分钟停 3 次则停用该扩展逻辑。
- 放一份 `tsconfig.json`（参考 [examples/components](../../examples/components)），用 `gs types <文件夹>` 生成类型、`gs check <文件夹>` 检查（需 `src/main.ts` 或 `src/main.js`）。

## 面板与界面位置

脚本输出声明式界面树，由 anmacgamer 用原生控件渲染并回传事件。面板在 `contributes.panels` 中声明，需扩展点 `panel` 与 `script`（[清单参考](manifest.zh-CN.md)）。

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

- `render(tree)` 更新界面（`render(null)` 清空）。上限：256 KiB、2000 节点、16 层、每秒最多 30 次。错误：`GS-API-075`（树非法）、`GS-API-076`（未为该脚本声明面板）。
- 节点：`column`、`row`、`group`、`text`（最多 4000 字符；`format: "markdown"` 支持粗体/斜体/代码/列表，无链接图片）、`button`、`field`、`number`、`toggle`、`choice`、`slider`、`list`、`table`（最多 16 列 200 行）、`chart`、`canvas`、`image`（包内 <= 2 MiB PNG/SVG）、`progress`、`separator`、`deviceItem`、`steps`、`qr`、`keyValue`。交互节点须有 `id`。界面文字不经过 `l10n`。
- 事件：`click`、`change`（含 `value`）、`submit`、`select`、`open`、`close`，回调收到 `{ id, value, signal }`。

| `where` | 脚本接口 |
|:--|:--|
| `mirror`、`page`、`settings`、`extension`、`device` | `ui.panel(名字).render()` / `.on()`（`device` 的 `open`/`close` 传设备 `id`；`settings` 仅在该节打开时触发） |
| `menu` | `ui.menu(项)` 与 `ui.menu.on("select", …)` |
| `status` | `ui.status(名字).render(树)`（最多 64 节点、100 字符） |
| `toolbar` | `ui.toolbar(名字).set({ title, icon, checked, enabled })`（`set(null)` 重置）、`.on("click", …)`；同名 `ui.panel` 会在按钮下方弹出 |
| `contextMenu` | `ui.contextMenu(名字).set(项)`（最多 16 项）、`.on("choose", ({ id, value, target }) => …)`（`target`：`{ kind: "device" \| "profile", id, name }`） |
| `tray` | `ui.tray(名字).set(项)`（最多 16 项）、`.on("choose", …)` |

菜单项为 `{ id, title, icon, checked, enabled }`（`checked` 点击切换 `value`）。图标支持内置名，面板、工具栏与 `deviceItem` 还支持 `svg:<包内路径>`。

## 程序事件

```ts
events.on("profile.applied", ({ value }) => log.info("profile applied", JSON.stringify(value)));
```

`events.on(主题, 处理函数, 选项)` 为每个事件启一个任务，传入 `{ topic, value, signal }`。主题见[原生扩展指南](native.zh-CN.md#程序事件)与 [sdk/docs/events.md](../../sdk/docs/events.md)。
- 用户逻辑收全部事件；扩展逻辑总能收 `extension.enabled`、`extension.disabled`、`settings.changed`（本扩展设置、语言与外观）。
- 会话脚本收本机事件；跨手机事件（及应用脚本/任务收手机事件）需 `events.app`（无权限则静默不送）。
- 原生 `emit_event` 自定义主题：本扩展写 `<主题>`，依赖扩展写 `"<扩展 id>:<主题>"`（须在 `dependencies` 中）。
- 限流：每主题每秒前 100 条立刻送达，其余合并为该秒最后一条。

## 扩展之间的服务

在 `contributes.services` 声明并在应用脚本注册：

```ts
ext.provide<{ track: string }>("best", ({ track }, { caller }) => best.get(track) ?? null);
```

调用方在 `dependencies` 列出提供方（含可选依赖）后调用：

```ts
const ms = await ext.call<number | null>("alice.lap-timer/best", { track: "monza" });
```

- 名称：`<扩展 id>/<服务名>`。扩展逻辑只能调自身或声明依赖的服务（否则 `GS-API-147`）；用户逻辑可调任意已启用扩展。
- 处理函数收 `(参数, { caller, permissions, signal })`（用户逻辑 `caller === ""`；取消或 10 秒超时中止 `signal`）。返回即结果（无返回为 `null`），抛错即失败。
- **权限取双方交集**：缺权限报 `GS-API-072`。
- 参数与结果为 <= 252 KiB JSON，按清单 `input`/`output` 校验（支持 `type`、`properties`、`required`、`items`、`enum`、`minimum`、`maximum`、`maxLength`、`title`、`description`）。单个逻辑最多 256 个待决调用。
- 错误：`GS-API-145`（未声明）、`GS-API-146`（无提供方或 10 秒未 `provide`）、`GS-API-147`（未声明依赖）、`GS-API-148`（schema/JSON/大小）、`GS-API-149`（抛错、超时或超 256 个）。原生节点也可提供服务（[原生扩展指南](native.zh-CN.md#服务)）。

## 本机传入连接

`net.listen(端口, 处理函数, { origins })` 在 `127.0.0.1` 监听 HTTP（需 `network.listen`，可在扩展详情关闭）。

```ts
const server = await net.listen(8765, async (req) => {
    if (req.method !== "POST") return { status: 405, headers: { allow: "POST" }, json: { error: "send POST" } };
    return { json: { ok: true, body: req.json() } };
});
```

- 端口 1024–65535，每扩展最多 4 个服务。错误：`GS-API-100`（无权限/已关）、`GS-API-101`（端口/来源错）、`GS-API-102`（占用或超 4 个）。
- 请求：`{ method, path, query, headers, text, bytes, json() }`（`path` 已解码，头小写）。响应：无返回（204）、字符串（200 纯文本）或 `{ status, headers, text \| json \| bytes }`。抛错回 500，超 10 秒回 504。
- 安全：仅绑 `127.0.0.1`；`Host` 须为 `127.0.0.1:<端口>` 或 `localhost:<端口>`；带 `Origin` 仅放行同源与 `origins`（最多 16 个 `https://...`，自动应答预检），拒绝 `Origin: null`。上限：头 16 KiB、正文 1 MiB、并发 16 连接、单次应答关闭、仅 HTTP。修改操作只用 `POST`。
- 关闭开关后服务关闭（`server.isOpen` 为 `false`），重开需重新 `net.listen`。示例：[deck-bridge](../../examples/extensions/deck-bridge)。

## 联网

```ts
const reply = await net.fetch("https://api.github.com/repos/wyb-070110/anmacgamer/releases/latest", {
    headers: { accept: "application/vnd.github+json" },
});
```

- `net.fetch(地址, { method, headers, body, timeoutMs })`：仅 HTTPS，返回 `{ status, url, headers, text, bytes, json() }`（头小写，`url` 为最终地址）。扩展逻辑需 `network` 且仅限清单 `hosts`（[清单参考](manifest.zh-CN.md#permissions)），最多 5 次重定向至允许主机。
- 上限：请求体 1 MiB、响应体 8 MiB、并发 8 请求、`timeoutMs` 1000–60000（默认 15000）。无 Cookie/缓存；禁设 `host`、`content-length`、`proxy-*`、`sec-*`；非法证书必败。
- `net.socket(地址, { protocols })`：`wss://` WebSocket（`NetSocket`：`send` <= 1 MiB、`close`、`onMessage`、`onClose`、`isOpen`、`protocol`），同 `hosts` 规则，最多 4 条。
- 错误：`GS-API-155`（权限/主机/重定向）、`GS-API-156`（协议/方法/头/正文/超时）、`GS-API-157`（大小/数量/积压）、`GS-API-158`（网络/证书/重定向/超时）、`GS-API-159`（连接已关）。

## 文件

```ts
const file = await files.pick({ filters: [{ name: "Key lists", extensions: ["keys"] }] });
const notes = await files.folder("notes");
await notes.write("today.txt", "Bring the map.");
```

- `files.pick({ save, filters, suggestedName })`（需 `files.pick`）：弹系统文件框，返回句柄（`read()`、`text()`、`write()`、`close()`，<= 64 MiB）或 `null`。
- `files.folder(名字)`（需 `files.paths` 并在清单 `folders` 声明；用户逻辑允许 1–32 位 `[a-z0-9-]`）：首次按 `purpose` 让用户选目录并记住。方法：`list(子目录)`（<= 4096 项）、`read(路径)`、`text(路径)`、`write(路径, 数据)`（原子写、自动建目录）、`remove(路径)`、`close()`。仅用 `/` 路径，拒绝 `..`、盘符和逃逸链接。
- 上限：每逻辑 64 个打开句柄、8 个等待对话框。
- 错误：`GS-API-160`（权限/未声明）、`GS-API-161`（选项/名/路径错）、`GS-API-162`（系统失败）、`GS-API-163`（大小/数量）、`GS-API-164`（已关闭/取消选择）。示例：[notes-panel](../../examples/extensions/notes-panel)。

## 朗读与语音命令

```ts
await speech.say("Ready", { rate: 1.1 });
const listener = await speech.listen(["open map", "close map"], async ({ phrase, confidence }) => {
    log.info("heard", phrase, confidence);
}, { confidence: 0.7 });
```

- `speech.say(文字, { engine, voice, rate, volume, interrupt })`（免权限）：1–1000 字符，读完返回 `true`，被打断/`speech.stop()`/取消返回 `false`。同引擎排队，除非 `interrupt: true`。
- `speech.listen(命令, 处理函数, { engine, language, confidence })`（需 `speech.listen`，否则 `GS-API-113`）：1–256 条命令（每条 1–64 字符，自由听写引擎可传 `null`），默认 `confidence` 0.6，回调收 `{ phrase, text, confidence }`，`listener.isOpen`。
- `speech.engines()` 列出 `system` 与 `<扩展 id>/<名字>`；`speech.voices({ engine })` 列声音；缺省取设置 > 音频。
- 错误：`GS-API-110`（引擎/能力/语言/声音）、`GS-API-111`（参数/上限）、`GS-API-112`（引擎失败）、`GS-API-113`（缺权限）。示例：[voice-commands](../../examples/extensions/voice-commands)。

## 硬件与自制设备

- `hardware.request({ kind, filters })`、`hardware.list()`、`hardware.open(id, 串口选项)`（独占打开）、`hardware.forget(id)`。需 `hardware`（`GS-API-080`），未勾选设备报 `GS-API-081`。
- `hardware.claim(资源, { wait, onWanted })` / `hardware.release(资源)` 协调硬件占用（`hardware.open` 自动占用）。
- `input.source(名字)`（需 `hardware`）：`press(键)`、`release(键)`、`axis(键, x, y)` 绑定为 `Ext.<扩展 id>.<名字>.<键>`（用户逻辑为 `Ext.mine.<名字>.<键>`）。应用脚本发往当前前台投屏窗口。

## 原生节点与长任务

挂在 `ext.<扩展>` 下（id 各段小驼峰）：

```ts
const version = ext.alice.board.version();
const job = ext.alice.board.flash({ port: "COM9" });
job.onProgress((fraction, text) => log.info(Math.round(fraction * 100), text));
job.onOutput((value) => log.info("found", JSON.stringify(value)));
const result = await job.result;
```

节点 100 毫秒内返回；任务返回 `ExtensionJob`（`result`、`onProgress`、`onLog`、`onOutput`、`follow(进度, 完成, 失败)`、`cancel()`），逻辑停止时自动取消。错误码见[原生扩展指南](native.zh-CN.md#逻辑里看到的错误)。

## 手机画面的帧

仅会话脚本（需 `media.read`）：`media.frames(处理函数, { fps, size, region })`（`fps` 0.1–10 默认 2，`size` 长边 64–1920 默认 640），传入 `{ image, time }`。错误：`GS-API-073`（应用脚本）、`GS-API-120`（参数）、`GS-API-121`（无画面）、`GS-API-122`（超 4 个订阅）。

## 识别

```ts
const lines = await vision.recognize("system", "ocr", { x: 0, y: 0.6, width: 0.4, height: 0.3 }, { language: "zh-Hans" });
```

- `vision.recognize(引擎, 种类, 区域, 参数)`（仅会话脚本，需 `device.screen`）：`种类` 为 `ocr`、`detect` 或 `classify`。取下一帧（0.25 秒无新帧用当前帧；2 秒无画面返回 `null`）。结果含归一化中心 `x, y`、`width, height` 与 `confidence`（0–1）。`区域` 可为 `res/resources.json` 名称或省略取全画面。
- `vision.engines()` 列出 `system`（参数 `language`、`words: true`）与 `<扩展 id>/<名字>`。`vision.engine(id)` 提供 `.read()`、`.detect()`、`.classify()`。扩展引擎单帧串行，5 秒超时。
- 错误：`GS-API-170`（引擎缺失/未启用）、`GS-API-171`（不支持种类/语言）、`GS-API-172`（参数错）、`GS-API-173`（引擎失败）、`GS-API-174`（超时/忙/队列满 16）。示例：[read-aloud](../../examples/extensions/read-aloud)；自写引擎见[原生扩展指南](native.zh-CN.md#识别引擎)。

## MCP 工具与 AI

- 应用脚本：`mcp.tool(名字, { description, input }, 处理函数)` 注册 `ext_<扩展 id>_<名字>`（点与连字符变下划线），每次调用需用户确认，60 秒超时，每扩展最多 32 个。
- `ai.ask(问题, { picture, timeoutMs })`：需 `ai` 权限及设置 > AI“允许逻辑调用 AI”，每分钟最多 10 次，应用脚本不可传 `picture`。提供方扩展不可用时抛 `GS-API-150`（[原生扩展指南](native.zh-CN.md#ai-服务提供者)）。

## 转换器

声明于 `contributes.converters`（[清单参考](manifest.zh-CN.md#converters扩展点-script最多-16-个)），脚本导出 `convert`（或默认导出），收 `ConvertInput`（`from`、`to`、`text`、`json()`、30 秒中止的 `signal`）并返回 `ConvertOutput`（`name`、`json`、`text`）。进出上限各 32 MiB。错误：`GS-API-165`（无导出）、`GS-API-166`（不可写）、`GS-API-167`（超 32 MiB）、`GS-API-168`（源文件不可读）。示例：[key-list](../../examples/extensions/key-list)。

## 功能引导

`contributes.tours`（扩展点 `tour`，[清单参考](manifest.zh-CN.md)）最多 4 段、每段 2–12 步，标题 <= 60 字符、正文 <= 300 字符。`start: "enabled"` 首次启用自动播放一次。目标不存在则跳过（先 `render` 面板）。详情页或 `action:tour:ext/<扩展 id>/<引导 id>` 可重放。`l10n` 键：`tours.<id>.title`、`tours.<id>.<步>.title`、`tours.<id>.<步>.text`（从 0 起）。

## 帮助文章

`contributes.help`（扩展点 `help`）最多 16 篇，可被帮助搜索与 AI `read_help` 读取。`file` 为英文 Markdown，`locales` 映射其他语言。支持 `#` 标题、列表（`1.` 粗体开头为步骤标题）、快捷键表（首列 `<kbd>`）、代码块、`> [!TIP]`/`[!NOTE]`/`[!WARNING]` 提示框（末行单链接变按钮）、包内相对图片 `![说明](images/pic.png)`（禁止 `:` 与 `figure:`），以及 `help:<文章>`、`help:ext/<扩展 id>/<id>`、`action:tour:ext/<扩展 id>/<引导 id>`、`https://` 链接。示例：[tour-and-help](../../examples/extensions/tour-and-help)。

## 常见错误码

| 代码 | 含义 |
|:--|:--|
| `GS-API-001`、`GS-API-073` | 无手机连接（应用脚本调用了手机接口） |
| `GS-API-072` | 缺少或未同意权限（或服务调用方缺少权限） |
| `GS-API-075`、`GS-API-076` | 界面树非法；面板未为该脚本声明 |
| `GS-API-080`–`GS-API-085` | 硬件：权限、未允许设备、不存在、打开失败、参数错 |
| `GS-API-100`–`GS-API-102` | 本机连接：权限/开关、端口/来源错、端口占用或超限 |
| `GS-API-110`–`GS-API-113` | 语音：引擎、参数/上限、引擎失败、权限 |
| `GS-API-120`–`GS-API-122` | 画面帧：参数、无画面、订阅超限 |
| `GS-API-145`–`GS-API-149` | 服务：未声明、不可用、未声明依赖、schema/大小、失败或超时 |
| `GS-API-150` | 设置 > AI 选中的扩展模型服务不可用 |
| `GS-API-155`–`GS-API-159` | 联网：权限/主机、参数错、大小/数量、不可达/证书/超时、连接已关 |
| `GS-API-160`–`GS-API-164` | 文件：权限/文件夹、参数/路径错、系统失败、大小/数量、已关/取消 |
| `GS-API-165`–`GS-API-168` | 转换器：无导出、不可写、超 32 MiB、不可读 |
| `GS-API-170`–`GS-API-174` | 识别：引擎、种类/语言、参数、失败、超时或忙 |
