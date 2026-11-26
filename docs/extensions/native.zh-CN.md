# 原生扩展指南

[English](native.md) · 简体中文

原生扩展是用 C/C++ 编写的 DLL（Windows）或 dylib（macOS），对接 [`sdk/include/gamescrcpy_ext.h`](../../sdk/include/gamescrcpy_ext.h)（`GS_EXT_ABI_VERSION` 2，由 `gs api write sdk` 生成）。适合直接操作硬件、每帧低延迟触控或提供自定义设备/连接/媒体/AI/识别引擎；画界面或调用常规接口请优先用[逻辑扩展](logic.zh-CN.md)。

## 最小示例

```cpp
#include <gamescrcpy_ext.h>
#include <string>

namespace {
const gs_ext_host* g_host = nullptr;

int32_t greet(void*, const gs_ext_value* args, uint32_t, gs_ext_result* result) {
    const std::string text = "Hello, " + std::string(args[0].text, args[0].size) + "!";
    return g_host->set_text(result, GS_EXT_TYPE_STRING, text.data(), text.size());
}

const uint32_t kGreetParams[] = {GS_EXT_TYPE_STRING};
const gs_ext_node kGreet = {sizeof(gs_ext_node), "greet", kGreetParams, 1, GS_EXT_TYPE_STRING, nullptr, &greet};
const gs_ext_contribution kContributions[] = {
    {sizeof(gs_ext_contribution), GS_EXT_KIND_NODE, "greet", &kGreet},
};
const gs_ext_plugin kPlugin = {sizeof(gs_ext_plugin), GS_EXT_ABI_VERSION, nullptr, 1, kContributions};
}

GS_EXT_EXPORT int32_t gs_ext_main(const gs_ext_host* host, const gs_ext_plugin** plugin) {
    if (!host || !plugin || host->abi_version != GS_EXT_ABI_VERSION) return GS_EXT_UNSUPPORTED;
    g_host = host;
    *plugin = &kPlugin;
    return GS_EXT_OK;
}
```

清单（`"requires": { "native": "^2.0" }`）：

```json
"native": {
  "dll": "native.dll",
  "nodes": [ { "name": "greet", "params": [ { "name": "name", "type": "string" } ], "returns": "string", "description": "Says hello." } ]
}
```

逻辑里调用 `ext.alice.hello.greet("world")`（id 各段小驼峰，如 `alice.lap-timer` -> `ext.alice.lapTimer`）。

## 入口与结构版本

- 导出 `gs_ext_main`。在 `unload` 调用前保持 `gs_ext_plugin` 及其指向数据有效。旧入口 `gs_ext_entry`（ABI 1）不再加载，需用新 SDK 重编并写 `"native": "^2.0"`。
- 由扩展填写或含数组的结构以 `uint32_t size = sizeof(...)` 开头以实现前后兼容；固定数组元素（`gs_ext_value`、`gs_ext_contact` 等）无 `size`。
- 后加功能先用 `supports("<功能>")` 检查。2.1.0 / 2.2.0 支持：`jobs`、`events`、`settings`、`claims`、`job-output`、`app-events`、`input-sources`、`services`、`transports`、`media`、`frame-taps`、`vision-engines`、`devices`。

## 贡献表

`gs_ext_plugin.contributions` 最多 256 项 `gs_ext_contribution { size, kind, name, data }`，名称须与清单一致（未知 `kind` 记日志跳过）：

| `kind` | 常量 | 结构 | 清单 |
|:--|:--|:--|:--|
| `touch-method` | `GS_EXT_KIND_TOUCH_METHOD` | `gs_ext_touch_method` | `touchMethods` |
| `node` | `GS_EXT_KIND_NODE` | `gs_ext_node` | `nodes` |
| `attachment` | `GS_EXT_KIND_ATTACHMENT` | `gs_ext_attachment` | `attachments` |
| `behavior` | `GS_EXT_KIND_BEHAVIOR` | `gs_ext_behavior` | `behaviors` |
| `job` | `GS_EXT_KIND_JOB` | `gs_ext_job` | `jobs` |
| `device-provider` | `GS_EXT_KIND_DEVICE_PROVIDER` | `gs_ext_device_provider` | `deviceProviders` |
| `input-source` | `GS_EXT_KIND_INPUT_SOURCE` | `gs_ext_input_source` | `inputSources` |
| `transport` | `GS_EXT_KIND_TRANSPORT` | `gs_ext_transport` | `transports` |
| `video-source` | `GS_EXT_KIND_VIDEO_SOURCE` | `gs_ext_video_source` | `videoSources` |
| `audio-source` | `GS_EXT_KIND_AUDIO_SOURCE` | `gs_ext_audio_source` | `audioSources` |
| `media-sink` | `GS_EXT_KIND_MEDIA_SINK` | `gs_ext_media_sink` | `mediaSinks` |
| `recording-exporter` | `GS_EXT_KIND_RECORDING_EXPORTER` | `gs_ext_recording_exporter` | `recordingExporters` |
| `frame-tap` | `GS_EXT_KIND_FRAME_TAP` | `gs_ext_frame_tap` | `frameTaps` |
| `vision-engine` | `GS_EXT_KIND_VISION_ENGINE` | `gs_ext_vision_engine` | `visionEngines` |

语音引擎与 AI 服务提供者在清单中由节点和长任务组合。`gs_ext_plugin` 可选回调：`unload`、`settings_changed`、`claim_wanted`、`on_event`、`device_event`（不用填 `NULL`）。

## 宿主函数

字符串均为带长度 UTF-8（除非注明以零结尾）。状态码（`gs_ext_status`）：`GS_EXT_OK`、`GS_EXT_FAILED`、`GS_EXT_UNSUPPORTED`、`GS_EXT_DENIED`、`GS_EXT_BAD_ARGUMENT`、`GS_EXT_BUSY`。

| 函数 | 用途 |
|:--|:--|
| `log`、`now_us`、`supports` | 写日志、单调微秒时钟、功能检测 |
| `store_get`、`store_set` | 持久化 JSON 存储（总 <= 1 MiB，嵌套 <= 64 层） |
| `allowed`、`notify` | 权限检查；系统通知（需 `notify`） |
| `set_text`、`set_error` | 向 `gs_ext_result` 写入文本结果或错误 |
| `job_progress`、`job_log`、`job_finish`、`job_fail`、`job_output` | 长任务生命周期与中间输出 |
| `emit_event`、`events_subscribe` | 发自定义事件；订阅程序事件（`on_event`） |
| `setting_get`、`devices_changed` | 读清单设置；刷新设备列表 |
| `claim`、`release`、`input_event`、`call_service` | 硬件占用、输入事件、跨扩展服务调用 |
| `transport_changed`、`stream_opened`、`stream_data`、`stream_writable`、`stream_closed` | 连接方式字节流 |
| `sources_changed`、`video_frame`、`audio_samples`、`stream_ended` | 音视频来源 |
| `device_list`、`device_open`、`device_write`、`device_control`、`device_close` | 经宿主代理的串口/HID/BLE 读写 |

## 线程、时间与进程隔离

每个扩展独立运行在 `gs-exthost.exe`（macOS 为 `gs-exthost`）并通过共享内存通信。节点超时 > 2 秒或通道断开会终止宿主进程；1 分钟崩溃 3 次自动停用并写崩溃报告（minidump/`crash.txt`）。开发期可用 `gs ext test <文件夹> --hosted` 测独立进程（[测试扩展](testing.zh-CN.md)）。

| 回调 / 调用 | 线程与时间规则 |
|:--|:--|
| 节点 `call` / 设备提供者 `list` | 单库串行，100 ms 内返回 |
| 触控方式 `open`/`close`/`frame`/`key`… | 单线程；`frame` 在实时线程，立即返回 |
| 附加 / 行为回调 | 实时线程禁阻塞；独立进程单次限 2 ms；10 秒失败 3 次停用扩展 |
| 长任务 `start` | 立即返回，在自有线程干活 |
| `settings_changed` / `claim_wanted` | 宿主线程 / 1 秒内回答 |
| `on_event` / `device_event` | 非界面线程有序投递，需快返（积压超 256 / 1024 丢弃） |
| `call_service`、`claim`（`GS_EXT_CLAIM_WAIT`） | 仅限长任务或自有线程（不可在宿主回调内调用） |
| `device_list`/`open`/`write`/`control` | 仅限自有线程（最多等 20 秒；宿主线程返回 `GS_EXT_UNSUPPORTED`） |
| 识别引擎 `run` | 专用非界面线程单帧串行（逻辑 5 秒超时） |
| `input_event`、`emit_event`、`job_*`、`device_close` | 任意线程 |

## 系统节点、触控方式、附加与行为

- **系统节点**（`gs_ext_node`）：收到已校验 `args`，写 `result->value`（或 `set_text`）返回 `GS_EXT_OK`，失败用 `set_error` + `GS_EXT_FAILED`（逻辑收 `GS-API-062`）。缺 `nodes[].permissions` 直接报 `GS-API-063`。
- **触控方式**（`gs_ext_touch_method`）：`open` 收 `gs_ext_touch_open`（序列号 + JSON `{ settings, deviceSettings, phone, device, touchBox }`）。`frame` 收画面比例与自然方向比例触点。`key`/`text`/`mouse`/`gamepad` 可为 `NULL`。非 `GS_EXT_OK` 回退普通触控。
- **附加**（`gs_ext_attachment`）：`input`、`output`、`wake`；`start` 收参数 JSON（含 `@position`、`@unit`）与确定性随机种子。
- **行为**（`gs_ext_behavior`）：自定义键位组件，含 `input`、`frame`（`wants_frames == 1`）、`wake`。

## 长任务

```cpp
int32_t flashStart(void* context, uint64_t job, const char* args, size_t size);
void flashCancel(void* context, uint64_t job);
const gs_ext_job kFlash = {sizeof(gs_ext_job), "flash", nullptr, &flashStart, &flashCancel};
```

- `start` 收任务号 `job` 与 JSON `args`，立即返回 `GS_EXT_OK` 并在自有线程执行。
- 过程用 `job_progress`（0..1 或 -1，限 20 次/秒）、`job_log` 与 `job_output`（中间 JSON，限 100 次/秒，先查 `supports("job-output")`）。结束时**恰好调用一次** `job_finish`（<= 252 KiB，符合 `returns`）或 `job_fail`。
- `cancel` 请求停止（仍以 `job_fail`/`job_finish` 结束；退出最多等 3 秒）。并发最多 4 个（或清单 `most`，超出报 `GS-API-067`）。逻辑错误：`GS-API-065`（失败）、`GS-API-066`（取消）。

## 事件

`emit_event(context, 主题, json)`：主题 1–64 字节 `[a-z0-9.-]`，禁以 `device.`、`session.`、`profile.`、`app.`、`orientation.`、`recording.`、`window.`、`extension.`、`settings.` 开头。JSON <= 64 KiB（空串即 `null`）。每秒前 100 条立刻送达，其余合并为最后一条。本扩展监听 `events.on("<主题>")`，依赖扩展监听 `events.on("<扩展 id>:<主题>")`。

## 程序事件

`events_subscribe(context, 主题, 1|0)` 投递至 `on_event(context, 主题, json)`（未知报 `GS_EXT_UNSUPPORTED`，缺 `events.app` 报 `GS_EXT_DENIED`）：

| 主题 | 值 | 权限 |
|:--|:--|:--|
| `device.connected`、`device.disconnected` | `{ phone, name }` | `events.app` |
| `session.started`、`session.ended` | `{ phone, name, failed? }` | `events.app` |
| `profile.applied` | `{ phone, name, profile, profileName }` | `events.app` |
| `app.foreground` | `{ phone, name, package, game }` | `events.app` |
| `orientation.changed` | `{ phone, name, rotation }`（四分之一圈） | `events.app` |
| `recording.started`、`recording.ended` | `{ phone, name, path, durationMs? }` | `events.app` |
| `window.focused` | `{ phone, name, window: "mirror"\|"grid", focused }` | `events.app` |
| `extension.enabled`、`extension.disabled` | `{ id, name, version }` | — |
| `settings.changed` | `{ keys, values, extension, device }`（本扩展 + 语言/外观） | — |

## 设置

`setting_get(context, 键, 手机, value)` 返回 JSON 文本，同线程下次调用前有效（空 `手机` 为全局；未声明键报 `GS_EXT_BAD_ARGUMENT`）。`settings_changed` 收到实际变更键的 JSON 数组。

## 设备提供者

`gs_ext_device_provider { size, name, context, list, act }`（需 `devices.provide`）。`list` 用 `set_text` 写 <= 64 项 JSON：`{ id*, name* (<= 64 B), kind ("touchBox"), summary (<= 200 B), state (ready|busy|problem|offline), stateText (<= 200 B), icon, touchMethod, resource, capture, actions (<= 8: { id, title, panel?, job? }) }`。`id: "settings"` 为设置按钮，其余进“更多”（含字符串参数 `device` 的任务自动传入设备 id；无 `panel`/`job` 调 `act`）。变化时调 `devices_changed(context, 提供者名)`。

## 硬件占用

打开硬件前调 `claim(context, 资源, flags)`，用完 `release`。资源名不区分大小写：`serial:COM9`、`ble:AA:BB:CC:DD:EE:FF`、`hid:<路径>`、`usb:<vid>:<pid>:<序列号>`（十六进制）。被占返回 `GS_EXT_BUSY`；`GS_EXT_CLAIM_WAIT` 先经 `claim_wanted(context, 资源, 谁)` 询问（1 秒内返回 `GS_EXT_OK` 且 3 秒内 `release` 即让出）。停止时自动释放。与 AOA 及 `hardware.claim` 共用。

## 输入设备

`gs_ext_input_source { size, name }`（需 `input.provide` 与清单 `inputSources`）。任意线程调 `input_event(context, 设备, 槽, x, y, 微秒)`（按钮 `x` 0/1；轴 `x` -1..1；摇杆 `x,y` -1..1；时间 0 为现在；限 1000 次/秒，归零必达）。方案绑定 `Ext.<扩展 id>.<设备>.<槽>`。

## 服务

`call_service(context, "<扩展 id>/<服务>", 参数 JSON, result)`（仅自有线程，先查 `supports("services")`，10 秒超时）。错误：`GS_EXT_UNSUPPORTED`、`GS_EXT_DENIED`（未列 `dependencies`）、`GS_EXT_BAD_ARGUMENT`（schema）、`GS_EXT_FAILED`。用 `node` 提供服务时接单 `json` 参数 `{ caller, permissions, input }` 并返回 `json`（按 `permissions` 鉴权，勿只看 `allowed()`）。

## 语音引擎

清单 `speechEngines` 组合任务与节点（id `<扩展 id>/<名字>`，校验码 `speech-engine-invalid`）：
- `say` 任务：参数 `text`（string）、`voice`（string）、`rate`（number 0.5–2）、`volume`（number 0–1）。
- `listen` 任务：参数 `phrases`（json 数组或 `null`）、`language`（string）、`confidence`（number）；经 `job_output` 持续输出 `{ phrase, text, confidence }` 直至取消。
- `voices` 节点：无参返回 json `[{ id, name, language }]`。

## 连接方式

`gs_ext_transport`（需 `transport`，联网另加 `network`；先查 `supports("transports")`）：
- `discover` 在 100 ms 内写 `[{ id, name, model, detail }]`，变化调 `transport_changed`。
- `open(context, stream, gs_ext_stream_open*)` 为 `control`/`video`/`audio`/`bulk` 立即返回，20 秒内调 `stream_opened` 或 `stream_closed`。负责启动手机服务端并连到 `gs_ext_stream_open.session`。
- `send` 单次 <= 64 KiB（`GS_EXT_OK` 全收；`GS_EXT_BUSY` 未收，等 `stream_writable`）。手机来数据调 `stream_data`，断开调 `stream_closed` / `close`。

## 画面、声音、媒体输出与取帧

先查 `supports("media")` / `supports("frame-taps")`：
- **音视频来源**（`gs_ext_video_source`、`gs_ext_audio_source`，需 `media.capture`）：`list`/`open`/`close`，推送 `video_frame`（`GS_EXT_PIXEL_BGRA` 或 `GS_EXT_PIXEL_NV12`，2–4096 px）与 `audio_samples`（48 kHz 16 位/浮点 PCM，1–8 声道）；变化调 `sources_changed`/`stream_ended`。
- **媒体输出**（`gs_ext_media_sink`，需 `media.output`）：`open`、`frame`（加黑边 BGRA）、`audio`、`close`，复制后立即返回。
- **录像导出**（`gs_ext_recording_exporter`，需 `media.output`）：`start(context, job, 输入 MP4, 输出路径)` 按长任务执行（计入 4 任务上限）。
- **取帧**（`gs_ext_frame_tap`，需 `media.read`）：`open`（尺寸/旋转变化时重开），`frame` 在专用线程收 BGRA（仅留最新帧）并向 `gs_ext_overlay` 写最多 256 个形状（`keep = 1` 保留上一帧）。

## 经 anmacgamer 用设备

需 `hardware` 与 `supports("devices")`，与 `hardware.request` 共用授权列表：
- `device_list` -> `[{ id, kind: serial|hid|ble, name, present }]`。
- `device_open(context, id, 选项, &device, answer)`（串口选项 `{ baudRate, dataBits, parity, stopBits, flowControl, dtr, rts }`；自动占用硬件）。
- `device_write`（串口 <= 65536 B；HID `target: { report }` <= 4096 B；BLE `target: { service, characteristic, response }` <= 512 B）。
- `device_control`（`op`：`signals`、`sendFeature`、`getFeature`、`services`、`read`、`subscribe`；字节均用 base64）。
- `device_close`（任意线程，`unload` 安全）。事件到 `device_event`：`GS_EXT_DEVICE_DATA`、`GS_EXT_DEVICE_REPORT`、`GS_EXT_DEVICE_NOTIFY`、`GS_EXT_DEVICE_CLOSED`（`removed`/`failed`/`forgotten`）。

## 识别引擎

`gs_ext_vision_engine { size, name, kinds, context, run }`（单库最多 4 个，无需权限，先查 `supports("vision-engines")`）。`kinds` 位掩码（如 `1u << GS_EXT_VISION_OCR`）须覆盖清单 `visionEngines[].kinds`。`run` 收只读 BGRA `gs_ext_image`（1–4096 px，`stride >= 4 * width`，alpha 255）与 JSON `params`，经 `set_text` 输出 JSON 数组：`ocr` `{ text, x, y, width, height, confidence }`、`detect` `{ label, x, y, width, height, confidence }`、`classify` `{ label, confidence }`（归一化 0..1）。逻辑错误：`GS-API-170`..`174`。

## AI 服务提供者

清单 `aiProviders` 组合 `models` 节点与 `chat` 任务（需 `ai.provide`，联网另加 `network`）：
- `models`（无参）-> <= 256 个 `{ id (<= 256), name?, traits: { think, alwaysThinks, efforts (minimal|low|medium|high|xhigh|max), thinkingEffort, topEffortsThink, longContext, millionContext } }`。
- `chat`（`request` json）：`{ model, system, messages, tools, stream, maxTokens, think, effort, longContext }`（`messages` 角色：`user` 含 `text`/`pictures`，`assistant` 含 `text`/`toolCalls`/`thinkingBlocks`，`tools` 含 `toolResults`）。超约 64 KiB 改为 `{ "file": "<路径>" }`（受限模式可读）。
- 流式片段用 `job_output`（`{ text }`、`{ thinking }`、`{ toolCall }`）；结束用 `job_finish`（`{ text, thinking, thinkingBlocks, toolCalls, stop, usage: { input, output } }` 或 `{ problem, status, code }`）。2 分钟无输出失败。服务不可用报 `GS-API-150`。

## 受限模式

`"sandbox": "restricted"` 让 `gs-exthost.exe` 运行在独立 AppContainer 中：
- 禁联网、禁启子进程、禁写容器外文件（仅可写 `%LOCALAPPDATA%\Packages\GameScrcpy.Extension.<散列>\AC` 与 `Temp`，自身目录只读）、禁剪贴板/窗口/系统设置。
- 硬件仅经[经 anmacgamer 用设备](#经-anmacgamer-用设备)（需 `hardware` 权限，`claim` 亦然）。
- 上限：1 GiB 内存（Windows）、200 日志/秒、3 通知/10 秒、4 设备变化/秒、16000 输入事件/秒；事件须为合法 UTF-8 JSON。
- `gs ext test` 始终在 `gs-exthost.exe` + AppContainer（`gs-test:<id>`）中运行受限 DLL。Administrators 属主的便携版在普通权限下可能报错误 5。

## 多架构与构建（Windows）

- `dll` 支持 `{ "x64": "...", "arm64": "..." }`。用 Visual Studio 2022 编译，静态链接运行库（`/MT`），导出 `gs_ext_main`（`GS_EXT_EXPORT` = `__declspec(dllexport)`）。可用 `gs ext init native <文件夹>` 生成模板。

## macOS

在 `contributes.native` 使用 `dylib`（单通用路径或 `{ "arm64": "...", "x64": "..." }`；macOS 忽略 `dll`，Windows 忽略 `dylib`，可同时携带）。
- 用 Clang（Xcode 命令行工具）编译，导出 `gs_ext_main`（`GS_EXT_EXPORT` = `__attribute__((visibility("default")))`）。
- `gs validate` 检查 Mach-O 架构、`gs_ext_main` 与实际绑定导入（去符号无法隐藏）；`task_for_pid`、`mach_vm_write`、`thread_set_state`、`ptrace` 等按注入拒绝。
- 运行于 `gs-exthost`；崩溃写 `crash.txt`（信号、地址、库与偏移）与日志（无 minidump）。
- `"sandbox": "restricted"` 使用 macOS `sandbox_init`：自身目录与系统库只读，容器 `~/Library/Application Support/anmacgamer/Containers/GameScrcpy.Extension.<散列>`（即 `HOME`/`TMPDIR`）可读写，环境变量仅 `HOME`/`TMPDIR`/`PATH`/语言；禁联网与子进程；无 Windows 1 GiB 作业对象限制。

## 逻辑里看到的错误

| 代码 | 含义 |
|:--|:--|
| `GS-API-060` | 扩展未启用 |
| `GS-API-061` | 参数类型错、任务缺参数或嵌套超 64 层 |
| `GS-API-062` | 节点失败（`set_error` 原因） |
| `GS-API-063` | 缺少节点/任务所需权限 |
| `GS-API-064` | 节点超过 100 ms 未返回 |
| `GS-API-065` | 任务失败（`job_fail` 原因）或结果不符 `returns` |
| `GS-API-066` | 任务已取消 |
| `GS-API-067` | 达到并发任务上限 |
