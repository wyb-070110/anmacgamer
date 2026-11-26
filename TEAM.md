# GameScrcpy-macos 分工与协作规则（TEAM）

> 由 Claude（总协调）维护。用户只和 Claude 对接：有问题、要下载、要用户在 Mac 上点授权，都写进自己的日志里，由 Claude 汇总后去问用户。
> 现状、构建命令、代码红线见根目录 `HANDOFF.md` 与 `AGENTS.md`，开工前先读这两份。
> 仓库（2026-11-26 起）：`origin` = 私有的 `wyb-070110/anmacgamer`（代码和发布都在这里）；`public` = 旧的公开仓库
> `wyb-070110/GameScrcpy-macos`（不再推送）；`upstream` = `NuoRan/GameScrcpy`。只有 Claude 推送，且要用户同意。

## 1. 角色

| 成员 | 工作位置 | 分支 | 职责 |
|---|---|---|---|
| **Claude**（Claude Code 桌面版） | `~/GameScrcpy-macos` | `ccr-e32722c2-75w11u`（集成主线） | 总协调；关键路径开发；审查并合并其他成员的分支；维护 `HANDOFF.md`、`TEAM.md`；推送 |
| **Antigravity** | `~/GameScrcpy-macos-antigravity`（独立 git worktree） | `mac/antigravity` | 边界清晰的 macOS 原生模块开发，自己编译、测试、提交 |
| **Gemini 桌面版** | 不碰代码 | — | 调研、用户文档、方案对比；产出写到 `docs/handoff/gemini/` |

Antigravity 的模型：**Gemini 4 Carbon (Medium)**（Claude 系列在你的套餐里要额外付费，不用）。Gemini 的 5 小时额度用完就停下等刷新，不开 Overages。不要用 Orchestrator（它会同时开多个代理改文件，容易互相覆盖），不要用 Gemini 3.8 Flash（即将下线）和 GPT-OSS 120B。

## 2. 协作红线（违反会互相覆盖代码）

1. **各用各的文件夹。** Antigravity 只在 `~/GameScrcpy-macos-antigravity` 里改代码、编译、提交；不要在 `~/GameScrcpy-macos` 里改任何文件，也不要在那里 `git checkout` / `git switch`（那是集成主线，Claude 在用）。
2. **只提交自己改的文件。** 用 `git add <具体路径>`，不要 `git add -A` / `git commit -a`。
3. **不推送、不改别人的分支、不 rebase 主线。** 做完一项就提交到自己的分支，并在日志里写一句"可合并"；Claude 审查后合并进主线并推送。开新任务前先 `git merge ccr-e32722c2-75w11u` 跟上主线。
4. **任务表里写了"负责文件"。** 只改自己负责的文件；必须动别人负责的文件（例如共用的 `src/app/Info.plist.in`、`CMakeLists.txt`）时，改动要小，并在日志里写明改了哪几行。
5. **手机和程序是共用的。** 同一时间只有一个 GameScrcpy 在连手机。Antigravity 测试自己编出来的程序时，用独立的数据目录：`GS_DATA_DIR=~/GameScrcpy-data-antigravity`，以免和用户正在用的程序抢同一份设置和单实例锁。需要真机时在日志里说一声，由 Claude 安排。
6. **不联网下载新依赖。** 需要下载（例如 Universal 版 FFmpeg）时，在日志里写清楚包名、来源、大小和用途，由 Claude 问用户。
7. **用户可见文字一律英文**，再更新 `src/app/i18n/gamescrcpy_zh_CN.ts`（规则见 `AGENTS.md`）。

## 3. 日志

每个成员只写自己的日志文件，写在自己的工作位置里：

- Antigravity：`docs/handoff/antigravity.md`（在它的分支里，随合并进主线）
- Gemini：`docs/handoff/gemini/README.md` 与各篇调研
- Claude：根目录 `HANDOFF.md` 的"协同工作记录区"（合并后统一整理）

每条记录写：日期、做了什么、验证了什么（命令和结果）、没做完/没验证的、需要用户或 Claude 做的事。

## 4. 任务表

### Claude（关键路径）

| # | 任务 | 负责文件 |
|---|---|---|
| C1 | **键鼠与手柄捕获**：IOHIDManager 读键盘扫描码与鼠标原始位移，GameController 读手柄与振动；打通按键映射，处理"输入监控"授权 | `src/platform/src/mac/InputCapture.mm`、`KeyCodes.h` |
| C2 | 触控板滑动顺滑度、投屏窗口交互 | `src/app/MirrorInputItem.*` |
| C3 | 审查合并、推送、CI、延迟与性能问题 | 全部 |

### Antigravity（按顺序做）

| # | 任务 | 负责文件 | 完成标准 |
|---|---|---|---|
| A1 | **语音命令与录音**：`SystemSpeech::listen` 用 SFSpeechRecognizer（尽量本机识别）匹配给定短语，`hearFrom` 识别传入的 16 kHz PCM，`record` 用 AVSpeechSynthesizer 写出 16 kHz 单声道 PCM；在 `Info.plist.in` 加麦克风与语音识别用途说明 | `src/platform/src/mac/Speech.mm`、`src/app/Info.plist.in`（只加两个键） | 新增 macOS 单元测试覆盖 `record` 与短语匹配；`ctest --preset mac-app` 全过；日志写明需要用户授权的步骤 |
| A2 | **USB**：`listUsbPhones` 用 IOKit 列出安卓手机（序列号、名称、VID/PID，有 adb 接口时 `debugging=true`），`UsbWatcher` 用 IOKit 通知报告插拔，`UsbDevice` 的控制传输（AOA 用） | `src/platform/src/mac/Usb.mm` | 接上手机时能列出 `RFGL71YMDHD` 且 `debugging=true`；插拔触发回调；测试或日志给出实测结果 |
| A3 | **打包**：`packaging/macos/` 下的脚本把 `GameScrcpy.app` 做成可单独分发的包（`macdeployqt` 带上 QML 与 Qt 框架、签名、`.dmg`）；另写一份 Universal（arm64 + x86_64）构建方案：需要哪些依赖的 Universal 版、从哪里来、多大 | `packaging/macos/`、新文档 | 在另一个用户目录里打开 `.dmg` 里的程序能启动；Universal 方案只写方案，下载前经 Claude 问用户 |

### Gemini 桌面版（只出文档）

| # | 任务 | 产出 |
|---|---|---|
| G1 | **macOS 使用指南**（中文为主，附英文）：安装、首次打开时的系统授权（本地网络、输入监控、麦克风）、USB 与无线调试连接、常见问题 | `docs/handoff/gemini/macos-guide.md` |
| G2 | **调研：扩展滤镜在 Metal 上运行**。扩展滤镜是 Direct3D 的 HLSL 像素着色器（见 `src/ext/` 的 FilterSource 与 `docs/extensions/`）。比较 DXC/SPIRV-Cross、glslang 的 HLSL 前端、Qt ShaderTools 等方案：能否离线随程序发布、许可证、对现有滤镜的兼容程度、工作量 | `docs/handoff/gemini/research-filters.md` |
| G3 | **调研：Universal 版与公证**。FFmpeg、SDL3、OpenSSL 的 Universal 构建或获取方式，Developer ID 签名与公证（notarization）的要求与步骤 | `docs/handoff/gemini/research-universal.md` |

#### 第二批（G1–G3 已交，Claude 已读）

| # | 任务 | 产出 |
|---|---|---|
| G4 | **更新使用指南**：键鼠映射已不需要"输入监控"授权（读的是程序自己窗口里的按键，`ed4672b`）；手柄走 SDL3，支持 Xbox/PS4/PS5 与震动；投屏打字改由手机上的 GameScrcpy Companion 输入法接收，手机自带键盘不再弹出（第一次投屏会自动装 Companion）；赞助入口已删除；新增"工具栏位置"（上/下/左/右）。把过时的说法改掉，常见问题里加"打字时手机键盘还弹出来怎么办" | 改 `docs/handoff/gemini/macos-guide.md` |
| G5 | **调研：有线投屏延迟忽高忽低**。手机是三星 Galaxy Z Flip（SM-F971B，Android 16 左右），120 Hz 屏。已知：跟随手机帧率时编码器积压到 8 秒，限到 60 fps 后平均 5 ms、最大 33 ms，但用户仍觉得有线不稳。查：MediaCodec 低延迟相关键（`KEY_LATENCY`、`KEY_PRIORITY`、`KEY_OPERATING_RATE`、`KEY_MAX_B_FRAMES`、`KEY_INTRA_REFRESH_PERIOD`、三星/高通厂商私有键）、scrcpy 上游与社区对三星机型的经验、USB 2/3 与 adb 转发对抖动的影响、可以在电脑端测量的指标。每条建议写清：改哪个文件（`android/server/` 或 `src/`）、预期效果、风险 | `docs/handoff/gemini/research-latency.md` |
| G6 | **调研：触控板滑动映射成手机触摸怎样才顺**。现在双指滑动会变成一次单指拖动（`src/app/MirrorInputItem.cpp` 的 `swipeWithTrackpad`），用户说桌面左右滑仍有卡顿。查：macOS 触控板事件的频率与相位（began/changed/ended、惯性 momentum）、Android 判定 fling 的速度条件（`ViewConfiguration` 的最小/最大 fling 速度、触摸事件时间戳）、注入触摸时补点/插值/按时间戳发送的做法，以及 scrcpy、Vysor 等工具怎么做 | `docs/handoff/gemini/research-swipe.md` |
| G7 | **macOS 版更新说明草稿**（中英）：从 `git log` 里本分支的提交整理出用户看得懂的变化列表，不写内部细节 | `docs/handoff/gemini/release-notes-macos.md` |

G4 → G5 → G6 → G7 按顺序做，每篇开头写日期和结论，做完在 `docs/handoff/gemini/README.md` 记一笔。

#### 第三批

| # | 任务 | 产出 |
|---|---|---|
| G8 | **App 图标：你来定风格、出方案**。用户请你按用途来决定，不受现有标志限制。<br>**用途**：GameScrcpy for macOS 把安卓手机（以游戏为主）投到 Mac 上，用 Mac 的键盘、鼠标、触控板和手柄来玩、来操作手机；低延迟、原画质、高刷新率；还能做键位映射、自动化脚本和录屏。用户是想在 Mac 上玩手机游戏、或用 Mac 操作手机的人。软件界面是 macOS 26 的 Liquid Glass 风格，强调色是系统蓝 #0A84FF。<br>**限制**：macOS 26 及以后用 Icon Composer 分层图标（一个背景填充 + 最多 4 组前景图层；图层是 1024×1024 的透明 PNG 或 SVG；系统自己加圆角形状、边距、玻璃高光和阴影）；不能有文字；16 px 时还要认得出；不用安卓机器人、任何品牌标志或游戏角色。现在的临时标志是 `docs/images/logo.png`（横放的手机 + 准星），可以参考，也可以推翻。<br>**要交**：3–5 个方向，每个写清寓意、构图、颜色（具体色值）以及为什么适合这个用途；能出图就把概念图放进 `docs/handoff/gemini/icon/`；最后推荐一个，并把它拆成图层（背景色或渐变、每层画什么、位置和大小），让 Claude 照着做成 Icon Composer 图标 | `docs/handoff/gemini/icon.md`（+ `icon/` 概念图） |

#### 第四批（2026-11-26，Antigravity 没额度，用户请 Claude 和 Gemini 配合）

| # | 任务 | 产出 |
|---|---|---|
| G9 | **文档改成英文为主、去掉 AI 味，并改用新名字 anmacgamer**。项目已改名 **anmacgamer**（程序、窗口、DMG 都会叫这个名字；上游 NuoRan/GameScrcpy 的出处、许可证与致谢必须保留原样）。<br>**范围**：仓库根目录的 `README.md`、`README.en.md`、`CONTRIBUTING*.md`、`SECURITY.md`、`CODE_OF_CONDUCT.md`（行为准则正文是公约原文，只改前言）、`LICENSING.md`（只改文字，许可证事实一个字都不能变）、`CHANGELOG.md` 里本 macOS 版的部分、`docs/README.md`、`docs/macos-port.md`、`docs/architecture.md`、`docs/extensions/*.md`。<br>**要求**：① 英文版做主：`README.md` 变成英文，中文版改名 `README.zh-CN.md`（其他成对的 `*.en.md` 同理：英文去掉 `.en` 作主文件，中文加 `.zh-CN`），两份互相链接、内容一致。② 去 AI 味：不要套话、营销腔、排比、emoji、"强大/无缝/一站式"之类的词，不要每段都加粗；句子短，直接说能做什么、怎么做；功能列表只写已经做好、测过的（看 `git log` 和 `docs/macos-port.md`），没做完的写在"Not yet"里。③ 链接、代码块、命令、文件路径保持可用（改了文件名就把所有指向它的链接一起改）。<br>**不要动**：`protocol/SPEC.md`、`docs/handoff/` 下别人的文件、`src/` 下的任何文件（程序内帮助 `src/app/help/` 由 Claude 处理）、`TEAM.md`。<br>**交付**：直接改上面这些文件（不提交 git，Claude 审阅后提交）；在 `docs/handoff/gemini/README.md` 记一笔：改了哪些文件、重命名了哪些、哪里拿不准。 | 上述文件 |

Gemini 桌面版如果不能直接写文件，就把成文的 Markdown 交给用户，存到上面的路径即可，Claude 会读。
