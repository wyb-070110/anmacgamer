# 扩展开发

[English](README.md) · 简体中文

anmacgamer 扩展是一个含 `manifest.json` 的文件夹，打包为 `.gsext`（zip）。扩展可以只包含数据（主题、滤镜、方案、帮助），也可以包含 TypeScript 逻辑（运行在低权限子进程中），或基于 C 接口的原生 DLL/dylib。

| 文档 | 内容 |
|:--|:--|
| [清单参考](manifest.zh-CN.md) | `manifest.json` 字段、贡献项、`l10n` 与限制 |
| [逻辑扩展指南](logic.zh-CN.md) | 脚本、界面面板、事件、服务、网络、文件、语音、识别、转换器、硬件、引导与帮助 |
| [原生扩展指南](native.zh-CN.md) | C 接口（`gs_ext_main`）、宿主调用、长任务、事件、设置、提供者、媒体、AI、受限模式 |
| [测试扩展](testing.zh-CN.md) | `gs validate`、`gs check`、`gs ext test` 与应用内测试 |
| [打包与发布](publishing.zh-CN.md) | `gs pack`、签名、`gamescrcpy://` 链接、更新列表与 `.gsbundle` |

具体函数签名见 [sdk/docs](../../sdk/docs/README.md)（[sdk/types](../../sdk/types/gamescrcpy.d.ts)）与 [gamescrcpy_ext.h](../../sdk/include/gamescrcpy_ext.h)，均由 `gs api write sdk` 生成。

## 扩展能做什么

用到的每个扩展点都必须在 `requires` 中声明。也可直接从应用的方案页、组件库或任务设置导出 `.gsext` 作为起点。

### 数据

| 能力 | 清单字段 | 扩展点 |
|:--|:--|:--|
| 颜色主题（深色与浅色令牌） | `contributes.themes` | `theme` |
| 画面滤镜（HLSL 像素着色器） | `contributes.filters` | `filter` |
| 方案与方案补丁 | `contributes.profiles`、`contributes.profilePatches` | `profile` |
| 键位组件（TypeScript） | `contributes.components` | `component` |
| 定时或手动任务 | `contributes.tasks` | `task` |
| 帮助文章（帮助 > 扩展） | `contributes.help` | `help` |
| 功能引导 | `contributes.tours` | `tour` |
| 设置（全局或按手机分开） | `settings` | — |
| 名称与标题译文 | `l10n` | — |
| 界面语言包（可含帮助译文） | `contributes.locales` | `locale` |

### 逻辑

| 能力 | 清单 / API | 扩展点 / 权限 |
|:--|:--|:--|
| 会话（每台手机）或应用脚本 | `contributes.scripts` | `script` |
| 面板、菜单、工具栏、状态区、页面、托盘 | `contributes.panels` + `ui.*` | `panel` |
| 供其他扩展与逻辑调用的服务 | `contributes.services` + `ext.provide` / `ext.call` | — |
| 程序与设备事件 | `events.on` | `events.app`（跨手机/窗口） |
| HTTPS 与 WebSocket 请求 | `net.fetch`、`net.socket` | `network`（需列 `hosts`） |
| 用户选定的文件与文件夹 | `files.pick`、`files.folder` | `files.pick`、`files.paths`（需列 `folders`） |
| 本机 `127.0.0.1` HTTP 服务 | `net.listen` | `network.listen` |
| 朗读与语音命令 | `speech.say`、`speech.listen` | `speech.listen`（仅听取需要） |
| 串口、HID、BLE、自制按键 | `hardware.*`、`input.source` | `hardware` |
| 低帧率投屏画面 | `media.frames` | `media.read` |
| 文字与物体识别 | `vision.recognize`、`vision.engine` | `device.screen` |
| 方案/任务/录像转换器 | `contributes.converters` | `script` |
| 供 AI 客户端使用的 MCP 工具 | `mcp.tool` | — |

### 原生

声明在 `contributes.native` 下，扩展点为 `native`：

| 能力 | `contributes.native` 字段 | 权限 |
|:--|:--|:--|
| 触控方式 | `touchMethods` | — |
| 系统节点（`ext.<扩展>.<节点>()`） | `nodes` | 按节点声明 |
| 组件附加 / 自定义行为 | `attachments`、`behaviors` | — |
| 带进度可取消的长任务 | `jobs` | 按任务声明 |
| 设备提供者（设备页） | `deviceProviders` | `devices.provide` |
| 语音引擎（`speech.*`） | `speechEngines` | — |
| 自定义输入设备（按键与轴） | `inputSources` | `input.provide` |
| 手机连接方式（中转、隧道、云手机） | `transports` | `transport` |
| 画面与声音来源 | `videoSources`、`audioSources` | `media.capture` |
| 媒体输出 / 录像导出 | `mediaSinks`、`recordingExporters` | `media.output` |
| 全速取帧与叠加绘制 | `frameTaps` | `media.read` |
| 识别引擎（`vision.recognize`） | `visionEngines` | — |
| AI 服务提供者（设置 > AI、`ai.ask`） | `aiProviders` | `ai.provide` |
| 命令行子命令（`gs ext <扩展> <命令>`） | `contributes.cli` | — |

原生扩展还可读写存储、发通知、发事件、读设置、占用硬件、听程序事件与调用服务（见[原生扩展指南](native.zh-CN.md)）。设置 `"sandbox": "restricted"` 可在无系统权限的受限模式下运行。

## 扩展点与版本

`requires` 列出每个用到的扩展点及语义化版本范围（如 `"script": "^1.1"`）。次版本只增不改，向后兼容；主版本变更不兼容。

anmacgamer 2.2.0 提供的版本：

| 扩展点 | 版本 | 用于 |
|:--|:--|:--|
| `theme`、`filter`、`profile`、`help`、`tour`、`locale` | 1.0 | `themes`、`filters`、`profiles`/`profilePatches`、`help`、`tours`、`locales` |
| `component`、`task`、`script`、`panel` | 1.1 | `components`、`tasks`、`scripts`/`converters`、`panels`（同时需 `script`） |
| `native` | 2.1 | `contributes.native` |

保留扩展点（2.2.0 暂未提供）：`command`、`preset`（可安装但不会启用）。未在 `requires` 中声明的贡献项会导致 `gs validate` 报 `contribution-undeclared`。程序升级后不兼容的扩展会保持关闭，并在首次启动时集中提示“检查更新”和“打开扩展页”。

可选字段 `"gamescrcpy"` 标明编写时面向的程序主.次版本，仅用于卡片与安装框展示，兼容性仍由 `requires` 决定（见[清单参考](manifest.zh-CN.md#为哪个版本编写)）。

## 权限

扩展默认无任何权限。需在 `permissions` 中逐项声明并写明 `reason`（仅要求 `script`、`component`、`task` 或 `native` 的扩展可申请）。未声明权限在运行时被拒（逻辑报 `GS-API-072`，原生返回 `GS_EXT_DENIED`）。

| 权限 | 安装框提示 | 用途 |
|:--|:--|:--|
| `device.input` | 发送触控与按键 | `touch.*`、按键与文字输入 |
| `device.screen` | 读取手机画面 | `vision.*`、`vision.recognize` |
| `device.clipboard` | 读写手机剪贴板 | `clipboard.*` |
| `device.apps` | 查看与打开应用 | 前台应用、启动应用 |
| `devices.other` | 操作其他手机 | 跨手机操作；可选 `maxDevices`（1–64） |
| `mapping` | 触发控件、切换方案 | `controls`、`mapping.*`、`profile.*`、`globals` |
| `notify` | 发送系统通知 | `ui.notify`、原生 `notify` |
| `network` | 联网 | `net.fetch`、`net.socket`（需列 `hosts`，支持 `*.example.com` 与端口）；也用于校验原生网络导入 |
| `files.pick` | 打开你选择的文件 | `files.pick` |
| `files.paths` | 读写你为它选定的文件夹 | `files.folder`（需列 `folders`，首次使用时由用户选定） |
| `input.read` | 读取你的键盘和鼠标 | `input.cursor`、`input.isDown`、按键事件 |
| `hardware` | 访问你选中的设备 | `hardware.*`、`input.source` |
| `ai` | 使用你设置好的 AI 服务（会花你的额度） | `ai.ask` |
| `devices.provide` | 在设备页添加设备 | 原生 `deviceProviders` |
| `network.listen` | 接受本机程序的连接 | `net.listen`（可在详情中单独关闭） |
| `speech.listen` | 听你说的语音命令 | `speech.listen`（`speech.say` 无需权限） |
| `events.app` | 知道所有手机与窗口的动静 | 应用脚本、任务与原生的全设备事件 |
| `input.provide` | 把自制设备的按键送进键位 | 原生 `inputSources` |
| `transport` | 提供连接手机的方式 | 原生 `transports`（联网另加 `network`） |
| `media.capture` | 提供画面或声音 | 原生 `videoSources`、`audioSources` |
| `media.read` | 读取投屏的画面与声音 | `media.frames`、原生 `frameTaps` |
| `media.output` | 把画面或声音送到别处 | 原生 `mediaSinks`、`recordingExporters` |
| `ai.provide` | 提供 AI 模型服务 | 原生 `aiProviders`（联网另加 `network`） |

`adb.shell` 永不授予（写即报错）。原生库安装时会检查系统函数导入：未声明权限的网络/文件/输入/进程调用会显示提醒，进程注入类调用直接拒绝安装。

## 代码在哪里运行

- 数据：不执行代码（滤镜着色器在 GPU 执行，`gs validate` 会预先编译检查）。
- 逻辑：运行在低权限 `gs-scripthost.exe` 子进程，每次 API 调用均做权限校验。`"run": "session"` 按投屏手机各运行一份，`"run": "app"` 整个程序一份。界面由 anmacgamer 按声明树绘制。
- 原生：每个扩展独立运行在 `gs-exthost.exe` 进程中，崩溃不连累主程序。默认 `"sandbox": "full"` 拥有当前账户权限且安装检查更严；`"sandbox": "restricted"` 运行在 AppContainer（或 macOS 沙箱）中，无直接文件、网络或进程权限，见[受限模式](native.zh-CN.md#受限模式)。

## 安装、同意与更新

1. 安装：通过扩展页**从文件安装…**（`.gsext`/`.gsbundle`）或 `gamescrcpy://install` 链接（[打包与发布](publishing.zh-CN.md)）。安装前校验清单、路径、文件上限、杀毒（原生）、签名与导入。
2. 同意：用户启用并同意权限与依赖后，扩展才加载。
3. 更新：同 `id` 新版本覆盖旧版，保留设置与已同意权限。若新增权限、域名、文件夹或必需依赖，扩展会暂停（“重新打开，同意新的要求”）直至再次同意。
4. 签名：包可带 `SHA512SUMS`、`SHA512SUMS.minisig` 与 `authorKey`。首次安装显示公钥，后续同公钥版本显示“已验证的发布者”。已签名扩展拒绝无签名或换公钥的更新（换公钥需先卸载重装）。
5. 检查更新：含 `updates.index` 的扩展可手动检查，或在设置 > 通用开启后每天启动最多查一次（默认关）。仅在用户点击更新后下载，并校验大小、SHA-256、id、版本与公钥，见[更新列表](publishing.zh-CN.md#更新列表)。

## 出了问题

- 扩展卡片与详情列出清单、版本、依赖与权限同意问题。
- 独立进程 1 分钟内停 3 次，或附加/行为 10 秒内失败 3 次，扩展会自动关闭；重新启用可清除健康记录。
- 日志位于 `%APPDATA%\GameScrcpy\logs`（便携版 `data\logs`），前缀为 `[extensions] <id>:` 与 `logic <名字> on <手机>:`。**设置 > 关于 > 导出诊断包…**包含健康与崩溃记录。
- 可用扩展页**查找出问题的扩展**二分排查，或运行 `GameScrcpy.exe --safe-mode`（带扩展崩溃后下次启动也会自动进入安全模式）。

## 示例

位于 [examples/extensions](../../examples/extensions)（原生示例需先编译）：

| 示例 | 演示内容 |
|:--|:--|
| [crt-scanlines](../../examples/extensions/crt-scanlines) | 画面滤镜与设置 |
| [slate-theme](../../examples/extensions/slate-theme) | 颜色主题 |
| [pc-clock](../../examples/extensions/pc-clock) | 最小原生扩展（单个系统节点） |
| [deck-bridge](../../examples/extensions/deck-bridge) | `net.listen`、`input.source`、`ext.provide` |
| [voice-commands](../../examples/extensions/voice-commands) | `speech.listen` 与 `speech.say` |
| [event-log](../../examples/extensions/event-log) | `events.on` 记录程序事件 |
| [tour-and-help](../../examples/extensions/tour-and-help) | 功能引导与帮助文章 |
| [key-list](../../examples/extensions/key-list) | 纯文本键位表与方案双向转换 |
| [notes-panel](../../examples/extensions/notes-panel) | `files.folder` 与 `net.fetch` |
| [read-aloud](../../examples/extensions/read-aloud) | 会话脚本：`vision.recognize` 与 `speech.say` |
