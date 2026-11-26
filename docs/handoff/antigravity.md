# Antigravity 工作日志（`mac/antigravity`）

> 工作目录：`~/GameScrcpy-macos-antigravity` | 分支：`mac/antigravity`
> 状态：**可合并**

---

## [2026-11-25] 完成 A1（语音合成与语音命令识别）、合并主线、完成 A4（清理 Windows 遗留文案与中英翻译）及 G5 延迟方案评估

### 1. 完成 A1：macOS 语音命令识别与 16 kHz PCM 导出（提交 `2a59040`）

- **改动文件**：
  - `src/platform/src/mac/Speech.mm`：`SystemSpeech::voices()` / `languages()`（规范化 BCP-47 标签）、`say()` / `stop()`（`AVSpeechSynthesizer` 队列朗读与静默缓冲渲染）、`record()`（`AVSpeechSynthesizer writeUtterance:toBufferCallback:` + `AVAudioConverter` 重采样为 16 kHz 单声道 Int16 PCM；修复后台线程调用超时时 `SpeechLine` 按值捕获避免悬垂引用）、`hearFrom()` + `listen()` / `unlisten()`（`SFSpeechRecognizer`，支持时优先 `requiresOnDeviceRecognition = YES` 本机离线识别；支持中英文指令归一化、前后缀与前缀歧义消解，以及 `AVAudioEngine` 实时麦克风流式识别与分段重装）。
  - `src/app/Info.plist.in`（仅新增 2 个键，共 4 行）：新增 `NSMicrophoneUsageDescription` 与 `NSSpeechRecognitionUsageDescription`。
  - `src/platform/Posix.cmake`（改动 1 行）：`gs_platform` 链接 `-framework Speech`。
  - `tests/PortProbe.cmake`（改动 5 行）：`gs_platform_tests` 加入 `unit/platform/mac/SpeechMacTests.mm` 与 `-framework Speech`。
  - `tests/unit/platform/mac/SpeechMacTests.mm`（新增）：覆盖语音与识别语言列表、16 kHz 单声道 PCM 合成、从合成 PCM 识别匹配中文（“打开地图”）与英文（“Open Map”）指令并忽略无关语句（“今天天气很好”）、队列朗读与中途取消、非法声音与非法语言报错。
- **验证命令与结果**：
  - `cmake --preset mac-app && cmake --build --preset mac-app -j8`：编译通过（0 警告、0 错误）。
  - `python3 tools/check_source_rules.py`：`source rules: 0 violation(s)`。
  - `ctest --preset mac-app -R gs_platform_tests --output-on-failure -V`：33/33 用例（965 断言）全部通过，识别本机 180 个语音、62 种识别语言，中英文合成 PCM 离线识别与指令匹配均一次通过。
  - `ctest --preset mac-app -j4 --output-on-failure`：169/169 非真机用例通过（2 个真机用例跳过）。
- **需要用户在 Mac 上授权的步骤（首次使用实时麦克风语音命令时）**：
  1. 必须从签名后的 `GameScrcpy.app` 启动（命令行裸二进制无 `Info.plist` 用途说明，代码会安全返回提示而不会触发 TCC 崩溃）。
  2. 首次在方案或脚本中调用实时 `speech.listen`（非 `hearFrom` 测试缓冲）时，macOS 会依次弹出 **麦克风（Microphone）** 与 **语音识别（Speech Recognition）** 两个系统授权弹窗，请点击“好 / 允许”。
  3. 若曾拒绝授权，可前往 **系统设置 → 隐私与安全性 → 麦克风** 及 **语音识别** 中勾选开启 `GameScrcpy`（注：每次本地重编重签 `.app` 后，macOS TCC 可能需要重新确认）。

---

### 2. 同步主线 `ccr-e32722c2-75w11u`（合并提交 `26b5337`）

- **做了什么**：
  - 执行 `git merge ccr-e32722c2-75w11u`（合入 `ed4672b`..`dfeb2d2` 共 11 个主线提交）。
  - 解决唯一冲突 `tests/PortProbe.cmake`：同时保留本分支的 `SpeechMacTests.mm`（`-framework Speech`）与主线新增的 `InputCaptureMacTests.mm`（`-framework AppKit`）。
- **验证命令与结果**：
  - `cmake --preset mac-app && cmake --build --preset mac-app -j8`：通过。
  - `ctest --preset mac-app -j4 --output-on-failure`：170/170 非真机用例全部通过（2 个真机用例跳过）。
- **给 Claude 的备注（主线既有项，未触碰）**：
  - 合并进来的主线代码当前有 3 处 `check_source_rules.py` 提示（`src/app/CompanionLink.cpp:733`、`src/app/MirrorInputItem.cpp:47`、`src/app/PhoneScreenWatch.cpp:15`）以及 `src/ui/controls/ToolbarButton.qml:18-19` 的 3 条 `qmllint` 提示；按协作规则未改动 Claude 负责的文件与 `src/ui/controls/`。

---

### 3. 完成 A4：替换残留 Windows 文案并更新简体中文翻译

分 4 个小提交完成，未改动任何行为逻辑，也未触碰 `src/ui/theme/` 与 `src/ui/controls/`：

1. **C++ 设置、服务与错误建议文案**（`src/app/*.cpp`, `src/app/Settings.h`）
2. **QML 界面文案**（`src/app/qml/*.qml`）
3. **内置中英文帮助文档**（`src/app/help/index.json`, `src/app/help/en/*.md`, `src/app/help/zh_CN/*.md`）
4. **简体中文翻译更新**（`src/app/i18n/gamescrcpy_zh_CN.ts`，运行 `GameScrcpy_lupdate` 后人工逐条翻译全部 70 条新增词条，`unfinished` 计数为 `0`）

#### 修改对照表（文件 / 旧文案 / 新文案摘要）

| 文件 | 旧文案（摘要） | 新文案（摘要） |
|---|---|---|
| `src/app/AiAssistant.cpp` | `Windows Credential Manager did not keep/remove the key` | `macOS Keychain did not keep/remove the key` |
| `src/app/AoaSession.cpp` / `SessionController.cpp` | `no WinUSB driver` / `AOA needs the WinUSB driver...` | `AOA over USB is not available on macOS` |
| `src/app/DeviceManager.cpp` | `Needs a driver` / `composed by Windows` / `dropped by Windows` / `on the PC` / `running in the tray` / `WinUSB driver... Zadig` | `Not available on macOS` / `composed by macOS` / `dropped by macOS` / `on this Mac` / `running in the menu bar` / `AOA touch over USB is not available on macOS yet` |
| `src/app/DevicePresentation.cpp` | `Needs a driver` | `Not available on macOS` |
| `src/app/ExtensionHost.cpp` | `Windows Security found a threat` / `could not be scanned with Windows Security` | `macOS security checks found a threat` / `macOS has no on-demand malware scan API; Gatekeeper and XProtect check native libraries when they load.` |
| `src/app/ExtensionPanels.cpp` | `group in the tray menu` | `group in the menu bar item's menu` |
| `src/app/FailureAdvice.cpp` | `Install Android SDK Platform-Tools...` / `No usable graphics card... up-to-date graphics driver helps` | 补充 `Homebrew android-platform-tools`；改为 `Metal hardware acceleration is not available... running directly on a Mac with Metal support helps` |
| `src/app/FileAccess.cpp` | `Windows does not allow it` / `without .., a drive or characters Windows does not allow` | `macOS does not allow it` / `without .. or invalid path characters` |
| `src/app/McpTools.cpp` | `gs.exe did not finish / is missing / did not start` | `gs did not finish / is missing / did not start` |
| `src/app/PictureTouch.cpp` | `Needs this phone on a USB cable with the WinUSB driver` | `AOA touch over USB is not available on macOS yet; use Normal or UHID touch screen` |
| `src/app/Recordings.cpp` | `moved to the Recycle Bin` | `moved to the Trash` |
| `src/app/ScriptWorkbench.cpp` | `gs.exe did not start / is missing` | `gs did not start / is missing` |
| `src/app/ScriptWorkbenchFiles.cpp` | `missing next to gs.exe (typescript/tsc.exe)` | `missing next to gs (typescript/tsc)`（与 macOS `ScriptTools.cpp` 实际输出对齐） |
| `src/app/Settings.h` | `Following Windows` / `Close to the tray... notification area` / `Windows settings` / `speech: Windows` / `winusb zadig` / `dll` / `Videos folder` / `Windows proxy` | `Following macOS` / `Keep in the menu bar` / `System Settings > Accessibility > Display` / `speech: macOS` / 移除 `winusb zadig` / `dylib` / `Movies folder` / `macOS network proxy` |
| `src/app/SpeechEngines.cpp` | `.title = "Windows"`, `Windows has no voice`, `Windows only listens...` | `.title = "macOS"`, `macOS has no voice`, `macOS only listens...` |
| `src/app/VisionEngines.cpp` | `.title = "Windows"`, `Windows Settings, Time & language...`, `Windows could not read...` | `.title = "macOS"`, `macOS Vision text recognition...`, `macOS could not read...` |
| `src/app/qml/AboutPage.qml` & `Main.qml` | `...for Windows.` / `The driver is installed: %1...` | `...for macOS.` / `%1 can be controlled over AOA now` |
| `src/app/qml/AiSettings.qml` | `Kept in Windows Credential Manager under %1` | `Kept in the macOS Keychain under %1` |
| `src/app/qml/CaptureCardSheet.qml` & `DeviceCard.qml` | `needs the WinUSB driver` / `How to install the driver` | `AOA unavailable on macOS` / `About AOA on macOS` |
| `src/app/qml/ExtensionsPage.qml` | `Tray menu` | `Menu bar menu` |
| `src/app/qml/HardwarePicker.qml` | `Bluetooth is on in Windows` | `Bluetooth is on in System Settings > Bluetooth` |
| `src/app/qml/InstallDialog.qml` | `own Windows code` / `Windows Security found no threat` | `own native macOS code` / `Security check found no threat` |
| `src/app/qml/LanguageSetting.qml` | `follows Windows` | `follows macOS` |
| `src/app/qml/LogicWindow.qml` | `Save all files (Ctrl+S)` | `Save all files (⌘S)` |
| `src/app/qml/ProfilesPage.qml` / `RecordingsDialog.qml` / `TaskSettings.qml` | `Recycle Bin` | `Trash`（中文对应“废纸篓”） |
| `src/app/qml/SettingsPage.qml` | `Follow Windows` / `On Windows USB debugging... WinUSB driver` / `How to install the WinUSB driver` / `named pipe... Windows account` | `Follow macOS` / `AOA over USB is not available on macOS yet...` / `About AOA on macOS` / `local Unix socket... macOS user account` |
| `src/app/qml/ShortcutRow.qml` | `Keys without Ctrl, Alt or Win...` | `Keys without Command, Option or Control...` |
| `src/app/qml/SpeechSettings.qml` | `Windows has no voice / speech recognizer; add under Time & language, Speech` | `macOS has no voice / speech recognizer; System Settings > Accessibility > Spoken Content / Dictation / Speech Recognition` |
| `src/app/qml/TasksPage.qml` | `leaves GameScrcpy in the notification area` | `leaves GameScrcpy in the menu bar` |
| `src/app/help/index.json` & `en,zh_CN/aoa-driver.md` | WinUSB + Zadig 驱动安装教程、设备管理器卸载、Windows 防火墙 | 明确说明 macOS 无需驱动且暂未支持 USB AOA（推荐普通/UHID 或触控盒），保留无画面控制窗与扫码连伴侣说明，防火墙改为“本地网络与防火墙” |
| `src/app/help/en,zh_CN/capture-card.md` | Windows 相机应用、WinUSB 驱动链接 | 注明 macOS 暂未支持采集卡输入（标准 UVC 说明）及 AOA 状态 |
| `src/app/help/en,zh_CN/companion.md` | WinUSB 链接、Windows 防火墙 | `AOA touch over USB` 链接、`系统设置 > 隐私与安全性 > 本地网络` 与 macOS 防火墙 |
| `src/app/help/en,zh_CN/extension-features.md` | Windows 语音/设置、跟随 Windows、托盘菜单 | macOS 语音与权限说明、跟随 macOS、菜单栏图标菜单 |
| `src/app/help/en,zh_CN/extensions.md` | `GameScrcpy.exe`、Windows DLL、Windows 安全中心、画面滤镜未注 | `GameScrcpy.app` / `open -a GameScrcpy --args --safe-mode`、`.dylib` 动态库、Gatekeeper/XProtect 说明、注明扩展画面滤镜暂不支持 |
| `src/app/help/en,zh_CN/faq.md` | 显卡驱动、`GameScrcpy.exe`、`%APPDATA%\GameScrcpy` + `portable.flag`、`<kbd>Ctrl</kbd>+<kbd>K</kbd>` | Homebrew 安装 `adb`、本地网络权限、Metal 硬件加速、`~/Library/Application Support/GameScrcpy` + `GS_DATA_DIR`、`<kbd>⌘ Command</kbd>+<kbd>K</kbd>` |
| `src/app/help/en,zh_CN/keymap-editor.md` | `<kbd>Ctrl</kbd>` / `<kbd>Alt</kbd>` / `Ctrl+C` / `Ctrl+V` | `<kbd>⌘ Command</kbd>` / `<kbd>⌥ Option</kbd>` / `⌘C` / `⌘V` |
| `src/app/help/en,zh_CN/profiles.md` | `Recycle Bin` / `File Explorer`（回收站 / 资源管理器） | `Trash` / `Finder`（废纸篓 / 访达） |
| `src/app/help/en,zh_CN/scripts.md` | `Ctrl` / `Alt`、`gs.exe` / `.\gs.exe` | `⌘ Command` / `⌥ Option`、`GameScrcpy.app/Contents/MacOS/` 下的 `./gs` |
| `src/app/i18n/gamescrcpy_zh_CN.ts` | 70 条变更词条 | 全部补齐自然中文翻译（`unfinished` = 0） |

#### 存疑与留给 Claude 决定的边界点（未擅改）

1. **脚本 API 生成表 `src/script/src/Api.cpp` 与 `sdk/`**：`gamescrcpy_zh_CN.ts` 中 `[ScriptApi]` 上下文来自 `sdk/i18n/api-texts.js`（共 19 条提到 `Windows` 或 `tray menu`）。因改动 `Api.cpp` 并运行 `gs api write sdk` 会跨出 `src/app` 批量重生成 `sdk/` 下的类型与接口文档，本次保持未动，若确认需要一并改为 macOS，可改 `src/script/src/Api.cpp` 后执行 `gs api write sdk` 并跑 `GameScrcpy_lupdate`。
2. **键帽显示格式化函数 `Commands::keyCaps`（`src/app/Commands.cpp:150-161`）与 `KeyLabels.cpp:27-28`**：
   - `Commands::keyCaps(QKeyCombination)` 目前把 `Qt::ControlModifier` / `AltModifier` / `MetaModifier` 格式化为字符串 `"Ctrl"` / `"Alt"` / `"Win"` 送给 `Keycap.qml` 显示（macOS 上 Qt 的 `ControlModifier` 实际是 `⌘ Command`，`AltModifier` 是 `⌥ Option`，`MetaModifier` 是 `⌃ Control`）；`MenuItem.qml` 的 `shortcut` 属性也是直接渲染字符串（如 `"Ctrl+D"`）而非经 `QKeySequence(NativeText)` 转换。因涉及快捷键展示逻辑与 `src/ui/controls/MenuItem.qml`，按今晚规则未碰，交由 Claude 在 Liquid Glass UI 改造时统一决定是否改为 `⌘` / `⌥` / `⌃`。
3. **未纳入 macOS 构建的旧单元测试常量**：`tests/unit/app/`（当前仅在 `WIN32` 分支编译，不在 `PortProbe.cmake` 中）里有若干断言旧字符串（如 `SpeechEnginesTests.cpp` 断言 `title == u"Windows"`、`PictureTouchTests.cpp` 断言 WinUSB 文案）；将来若把 `gs_app_tests` 接入 `PortProbe.cmake`，同步更新那几个断言即可。

---

### 4. G5 有线投屏延迟调研（第 2–4 项）优先级评估（仅写结论，未改代码）

对照 `docs/handoff/gemini/research-latency.md` 第 2–4 项与 `android/server/src/main/java/com/gamescrcpy/server/video/MediaCodecSessionFactory.java` 现有实现，**建议最先尝试第 2 项（同时顺手带上第 4 项的编码器名与实际 `outputFormat` 日志），暂缓第 3 项（CBR）**，理由如下：

1. **为什么先做第 2 项（修正 `KEY_OPERATING_RATE` 并覆盖非高通编码器）**：
   - 看 `MediaCodecSessionFactory.java:211-224` 与 `286-296`：目前只有 `qualcomm(attempt.encoder())` 为真时才进入低延迟分支，且把 `KEY_OPERATING_RATE` 硬编码为 `Short.MAX_VALUE`（`32767`），并与私有键 `vendor.qti-ext-enc-low-latency.enable = 1` 绑在**同一个** `try` 块里。
   - 这在三星机型上有两个直接隐患：
     - **若该机采用高通编码器（`c2.qti.*`）**：传 `32767` 会让折叠屏受限散热下的 VPU 锁满频运行，极易触发温控/DVFS 周期性降频抖动；或者若三星 One UI 的 `c2.qti.*` 拒收 `32767`，`catch` 会 `codec.reset()` 回退到普通 `format()`，**连同 `KEY_QTI_LOW_LATENCY = 1` 一起丢掉**。
     - **若该机采用三星 Exynos 编码器（`c2.exynos.*` / `c2.sec.*`）**：`qualcomm()` 返回 `false`，**全程根本没有设置 `KEY_OPERATING_RATE`**！VPU 默认停留在省电调频档，桌面静止后一旦滑动，编码器升频唤醒就会产生 5 ms → 33 ms 的典型延迟毛刺。
   - 将 `KEY_OPERATING_RATE` 改为实际目标帧率 `keys.frameRate`（如 60 或 120），放进通用硬编配置并与高通私有键解耦（各自独立 fallback），收益最直接且几乎零画质副作用。
2. **为什么顺手配合第 4 项的日志**：
   - `formatSummary(codec, ...)`（第 254 行）已经能读出 `KEY_LATENCY` 和 `KEY_OUTPUT_REORDER_DEPTH`，在 `open()` 启动编码器后打印一行 `attempt.encoder() + ": " + formatSummary(...)` 零风险，一次连接就能看清三星实机到底选了哪个编码器、低延迟键有没有被拒收。
3. **为什么把第 3 项（`BITRATE_MODE_CBR`）放后面**：
   - CBR 在静止转快速滑动时虽能削平单帧体积尖峰，但会明显降低高复杂度画面（文字边缘、游戏细节）的画质，且部分 Android 硬件编码器对 HEVC/AV1 不支持 CBR；应先用第 2+4 项排除 VPU 调频与低延迟回退问题，若仍有大帧引发的 USB/adb 排队尖峰，再在 `EncoderCapabilities.isBitrateModeSupported(BITRATE_MODE_CBR)` 保护下做实验对比。

---

**当前分支状态：可合并（Ready to merge）**


---

## [2026-11-25] 下一批：合并主线 `dcbff67` 并完成 A2（macOS USB `libusb` 支持与 AOA 文案恢复）

### 1. 合并主线 `ccr-e32722c2-75w11u`（快进至 `dcbff67`）

- 执行 `git merge ccr-e32722c2-75w11u` 快进到 `dcbff67`，全量构建并运行 `ctest --preset mac-app -j4`：172/172 通过（2 个真机用例跳过）。
- 按最新指示跳过 A3（Claude 已完成打包）。

### 2. 完成 A2：macOS USB 手机枚举、热插拔与 AOA 控制传输（`src/platform/src/mac/Usb.mm`）

- **改动文件**：
  - `src/platform/Posix.cmake`：在 `APPLE` 分支为 `gs_platform` 链接 `libusb::libusb` 并定义 `GS_HAVE_LIBUSB`。
  - `src/platform/src/mac/Usb.mm`：
    - `usbLibraryVersion()`：返回 `libusb` 版本号（当前环境实测 `1.0.30`）。
    - `listUsbPhones()`：通过 `libusb` 枚举 USB 设备（排除 Hub 与 Apple `0x05AC` iPhone/iPad 设备），检查接口描述符识别 ADB 接口（`0xFF/0x42/0x01` → `debugging = true`）、MTP（`0x06/0x01/0x01`）、Google AOA 配件模式（`0x18D1:0x2D00..0x2D05`）及主流安卓厂商复合设备，优先通过 `libusb` 读取 UTF-16LE/ASCII 字符串描述符并在设备被占用时回退读取 IOKit `IOUSBHostDevice` 注册表属性，`driver` 恒为 `UsbDriver::Ready`，按 `serial` 排序去重。
    - `UsbDevice`：实现 `open`、`controlOut`（`0x40`）、`controlIn`（`0xC0`）、`maxPacketSize` 与 `failure` 错误映射，供 `AoaTouchSink` 发送 AOA v2 HID 控制传输。
    - `UsbWatcher`：独立 `libusb_context` + `libusb_hotplug_register_callback` + 独立事件线程（`"usb watch"`），析构时通过 `libusb_hotplug_deregister_callback` + `libusb_interrupt_event_handler` 零等待干净退出（无任何固定延时 sleep）。
  - `tests/unit/platform/mac/UsbMacTests.mm` & `tests/PortProbe.cmake`：覆盖 `usbLibraryVersion()`、`listUsbPhones()` 不变量与实机输出打印、`UsbDevice::open` 空/不存在设备返回 `UsbFailure::NotFound`（不触碰真实设备配件模式）、`UsbWatcher` 单次及连续多轮启停生命周期测试。
  - AOA 文案恢复（去除“macOS 暂未支持 AOA”占位说明，且不含任何 WinUSB/Zadig 字样）：
    - C++：`src/app/AoaSession.cpp`、`src/app/DeviceManager.cpp`、`src/app/DevicePresentation.cpp`、`src/app/PictureTouch.cpp`（未插线原因改为 `"Needs this phone on a USB cable"`）、`src/app/SessionController.cpp`。
    - QML：`src/app/qml/SettingsPage.qml`、`src/app/qml/CaptureCardSheet.qml`、`src/app/qml/DeviceCard.qml`。
    - 帮助与翻译：`src/app/help/index.json`、`src/app/help/{en,zh_CN}/aoa-driver.md`、`src/app/help/{en,zh_CN}/capture-card.md`、`src/app/i18n/gamescrcpy_zh_CN.ts`（`GameScrcpy_lupdate` 更新并补齐全部 12 条新译文，5732/5732 完成，`unfinished = 0`）。
- **验证命令与结果**：
  - `cmake --build --preset mac-app -j8 && cmake --build --preset mac-app --target all_qmllint`：0 编译警告、所改 QML 文件 0 `qmllint` 警告，所改文件 `check_source_rules.py` 0 违规。
  - `ctest --preset mac-app -R gs_platform_tests --output-on-failure -V`：39/39 用例（993 断言）通过；打印 `libusb version: 1.0.30`，当前运行测试时手机 `RFGL71YMDHD` 未插在 USB 口上（`adb devices -l` 与 `libusb` 均为 `attached USB phones: 0`）。
  - `ctest --preset mac-app -j4 --output-on-failure`：172/172 全部通过（170 通过，2 个真机测试跳过）。
- **给 Claude 与用户的真机 / AOA 验证步骤（未自行切手机配件模式）**：
  1. 插上手机 `RFGL71YMDHD` 后运行 `./build/mac-app/bin/gs_platform_tests -tc="*lists attached Android phones*" -s`，可查看 `listUsbPhones()` 输出的 `serial`、`name`、`vid/pid`、`driver=Ready` 与 `debugging=true/false`。
  2. **有画面 + USB AOA 触控测试**：保持手机 USB 数据线连接（开启 USB 调试或无线调试），启动 `GS_DATA_DIR=~/GameScrcpy-data-antigravity ./build/mac-app/bin/GameScrcpy.app/Contents/MacOS/GameScrcpy --touch aoa`，投屏后在镜像窗口点击/滑动/按键，日志应打印 `touch on <serial> goes through an AOA touch screen on the USB cable`（GameScrcpy 使用 AOA v2 HID EP0 控制请求 `51/54/55/56/57`，macOS 下无需重装驱动，也不抢占 `adb` 接口）。
  3. **无 USB 调试纯 AOA 控制窗测试**：在手机上关闭 USB 调试、仅插 USB 线，在设置中将触控方式设为 `AOA`，设备页会出现 `aoa-<serial>` 卡片（连接方式 `USB · AOA`），点连接后打开无画面控制窗口并在手机上看到触控/鼠标指针响应。
- **状态**：**可合并**


---

### 3. 完成 A5：macOS `/dev/cu.*` 串口设备枚举与 `termios` 读写（`src/hardware/`）

- **改动文件**：
  - `src/hardware/Posix.cmake`：`APPLE` 分支编译 `src/Worker.cpp`、`src/Worker.h` 与 `src/mac/SerialBackend.cpp`，链接 `-framework CoreFoundation` 与 `-framework IOKit`；非 Apple POSIX 系统继续保留 `NoDevicesBackend.cpp`。
  - `src/hardware/src/mac/SerialBackend.cpp`（新增）：
    - `listMacSerialPorts()`：通过 IOKit `IOSerialBSDClient` 枚举全部 `/dev/cu.*` 串口，沿 `kIOServicePlane` 向上查找父级 USB 节点提取 `idVendor`、`idProduct` 及 `USB Product Name` / `kUSBProductString`（无 USB 产品名时回退 `IOTTYBaseName`），生成 `id = "serial:" + path`，按 `id` 排序去重。
    - 热插拔监听：独立 `CFRunLoop` 线程（`"serial watch"`）注册 `IONotificationPortCreate`（`kIOFirstMatchNotification` + `kIOTerminatedNotification`），列表变化时触发 `watch` 回调；析构时通过 `CFRunLoopStop` 干净退出。
    - `openMacSerial` & `PosixSerialLink`：校验 `validSerialOptions`，以 `O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC` 打开串口并设置 `TIOCEXCL` 独占锁，配置 `cfmakeraw`、5–8 数据位、1/2 停止位、无/偶/奇校验、无/硬件（`CRTSCTS`）/软件（`IXON | IXOFF`）流控，标准波特率直接走 `cfsetspeed` + `tcsetattr`，高波特率（如 ESP32 触控盒常用的 `921600` / `2000000` 等最高 `4000000`）自动回退 `IOSSIOSPEED` `ioctl`；`TIOCMBIS`/`TIOCMBIC` 控制 DTR/RTS 信号线；独立读线程基于 `poll` + 唤醒管道零延时监听数据与拔线断开（`Closed::Removed`），写操作经 `Worker` 队列带超时发送（完全符合 R9 规则，0 sleep）。
  - `tests/unit/hardware/mac/SerialBackendTests.cpp` & `tests/PortProbe.cmake`：无需外接触控盒即可完整测试 IOKit `/dev/cu.*` 列表排序与多轮热插拔生命周期、非法参数/不存在端口/未支持设备类型的错误返回，以及基于 `openpty()` 伪终端的真实 `termios` 配置核验、`TIOCEXCL` 重复打开互斥、双向字节收发、DTR/RTS 信号设置与对端断开（模拟拔线 `Closed::Removed`）处理。
- **验证命令与结果**：
  - `cmake --preset mac-app && cmake --build --preset mac-app -j8`：0 警告、0 错误；所改文件 `check_source_rules.py` 0 违规。
  - `ctest --preset mac-app -R gs_hardware_tests --output-on-failure -V`：全部通过（本机枚举输出：`IOKit /dev/cu.* serial ports: 3; id=serial:/dev/cu.Bluetooth-Incoming-Port name="Bluetooth-Incoming-Port" path=/dev/cu.Bluetooth-Incoming-Port vid=0000 pid=0000; id=serial:/dev/cu.BoseQCUltra2HP name="BoseQCUltra2HP" path=/dev/cu.BoseQCUltra2HP vid=0000 pid=0000; id=serial:/dev/cu.debug-console name="debug-console" path=/dev/cu.debug-console vid=0000 pid=0000`）。
  - `ctest --preset mac-app -j4 --output-on-failure`：172/172 全部通过（170 通过，2 个真机测试跳过）。
- **未在真机硬件上验证的部分**：
  - 已通过 `openpty()` 验证完整 `termios` 与异步读写状态机；外接真实 USB 转串口触控盒（如 CH340 / CP2102 / ESP32-S3 高波特率 `IOSSIOSPEED` 路径及物理拔线）留待接入硬件时实测。
- **状态**：**可合并**


---

## [2026-11-25 午后] 合并主线、A6 图标（方向 B）、A7 触控轨迹工具

### 0. 合并主线 `ccr-e32722c2-75w11u`（提交 `e5fac65`）

- 冲突只在文件头（主线加了 SPDX 头）：`src/hardware/Posix.cmake`、`src/platform/src/mac/Usb.mm` 及 3 个新文件直接取 Claude 试合并分支 `merge/antigravity-a2-a5`（`44ecfca`）的版本，与 Claude 审过的结果一致。

### 1. A6：应用图标改为 Gemini 方向 B「双屏跃迁」

- **改动文件**：
  - `packaging/macos/icon/make_icon.swift`：改为只画方向 B 的三层（1024 全出血，全部在 164…860 内）：`phone.png` 海军蓝竖屏 `#1D3557→#0F2038`（188,264 260×480 r58）、`screen.png` 横屏玻璃 `#0A84FF→#38BDF8`（316,400 520×360 r52，压在手机右下）、`chevron.png` 白色粗折角 ›（线宽 48，圆头）。旧的 azure/ember/light 扁平变体与方向 A 图层删去。
  - `packaging/macos/icon/GameScrcpy.icon/icon.json` + `Assets/`：背景 `linear-gradient` `#F0F6FF→#CFE4FF`（上→下，键名对照 Xcode 模板与 IconComposerFoundation 里的 `linear-gradient`/`orientation`）；三组由上到下 chevron / screen（`"glass": true`，translucency 0.3）/ phone。
  - `packaging/macos/icon/build_icon.sh`（新增）：画图层 → `actool` 编译 `Assets.car` → 用 Icon Composer 自带的 `ictool` 渲染 16…1024 全部尺寸经 `iconutil` 打成 `GameScrcpy.icns`（actool 自己出的 `.icns` 只到 256 px，macOS 14/15 的大图标会糊）→ 导出 `GameScrcpy-1024.png`。
  - `src/app/resources/Assets.car`（新增）与 `GameScrcpy.icns`（替换）；`src/app/CMakeLists.txt` 把两者都放进 `Resources`；`src/app/Info.plist.in` 加 `CFBundleIconName = GameScrcpy`。
- **为什么提交二进制而不是构建时跑 actool**：编译 `.icon` 要 Xcode 26+，CI 在 `macos-15` 镜像上，Xcode 版本不保证；构建只拷贝结果，改图标时跑一次 `packaging/macos/icon/build_icon.sh` 即可。
- **验证**：
  - `actool` 无警告，输出 `Assets.car` + `GameScrcpy.icns`，partial plist 为 `CFBundleIconFile`/`CFBundleIconName = GameScrcpy`。
  - 构建后把 `GameScrcpy.app` 拷到新路径（避开 LaunchServices 缓存），用 `NSWorkspace.shared.icon(forFile:)` 渲染：macOS 27.2 上是完整的 Liquid Glass 图标，**没有灰底板**。`docs/handoff/glass/icon-b-finder-{16,32,128,512}.png`；16/32 px 仍能看出“浅底 + 深蓝竖块 + 亮蓝横块”，32 px 能看到折角。
  - 各外观（`ictool`，即 Icon Composer 的渲染器）：`icon-b-Dark.png`、`icon-b-ClearLight.png`、`icon-b-TintedDark.png`；macOS 14/15 用的 `.icns`：`icon-b-legacy-icns-256.png`。没有打开 Icon Composer 图形界面（避免在用户屏幕上弹窗），渲染用的是同一个引擎。
  - 深色外观下背景由系统换成近黑，海军蓝手机对比偏弱，但轮廓仍可辨；若要加强可在 Icon Composer 给 phone 组加 dark 专用填充。
  - `ctest --preset mac-app`：172/172（2 个真机测试跳过）。
- **状态**：**可合并**


---

### 2. A7：触控轨迹统计工具 `tools/touch_trace.py`

- **改动文件**：
  - `tools/touch_trace.py`（新增）：解析 `GS_TOUCH_TRACE`（`<steady µs> <s|x> <pointer> <action> <x> <y>`，其中 `x/y` 为 `0..65535` 归一化坐标），按每个指针从 `Down` 到 `Up`/`Cancel` 切分手势，逐条输出：
    - 移动帧间隔分布 `step p50/p90/max ms`；
    - 抬手前 100 ms 内的速度（严格对照 `docs/handoff/swipe-design.md` 中 Android `VelocityTracker` 规则：`ACTION_UP` 不计采样点、采样间隔 `> 40 ms` 清空历史、抬手距最后一次移动 `> 40 ms` 速度归零；默认输出 `scr/s`，支持 `--size WxH` 输出像素速度 `px/s`）；
    - 最后一次 `Move` 到 `Up` 的间隔 `lift gap`（ms）及手势分类（`fling` / `stopped`（`> 40 ms` 被安卓视作停住）/ `tap` / `cancel`），以及未发出帧计数 `unsent`；
    - 内置 `--self-test` 自测用例。
- **验证命令与结果**：
  - `python3 tools/touch_trace.py --self-test`：`touch_trace self-test passed`。
  - 独立 `GS_DATA_DIR` 演示副本实测（未触碰真机、未影响任何运行实例）：以 `--safe-mode --demo-device tests/data/video/emulator_h265.media` 加载临时测试配置，驱动纵向快滑（120 ms）、横向翻页滑（180 ms）、拖动后停顿 90 ms 再抬手、单击（30 ms）及双指捏合（200 ms），生成 62 行真实轨迹后运行 `tools/touch_trace.py`：
    - 两条快滑：`step p50 = 17.7 / 18.7 ms`，速度 `5.00 / 3.88 scr/s`（`--size 1080x2400` 下 `12002 / 4195 px/s`），`lift gap = 15.4 / 17.1 ms` → `fling`；
    - 拖动停顿后抬手：`13` 次移动，`lift gap = 88.7 ms > 40 ms`，速度 `0.00` → `stopped`；
    - 单击：`0` 次移动，`30 ms` → `tap`；
    - 双指捏合：双指针同步 `11` 次移动，速度 `0.75 scr/s`，`lift gap = 0.1 ms` → `fling`。
  - `ctest --preset mac-app`：172/172（2 个真机测试跳过）。
- **状态**：**可合并**


---

### 3. 文档重命名（`anmacgamer`）、标明上游出处与清理原作者/Windows 遗留痕迹

- **改动文件**：
  - 上游出处集中标注（仅此三处保留 `NuoRan/GameScrcpy` Apache-2.0 出处说明）：
    - `README.md`、`README.en.md`：项目名称改为 **anmacgamer**，顶部图标换为新 macOS 图标 `packaging/macos/icon/GameScrcpy-1024.png`，仓库与发布/反馈/安全链接统一指向 `https://github.com/wyb-070110/anmacgammer`，功能列表同步至当前 macOS 通用版状态（Apple 芯片 + Intel、Liquid Glass、键鼠手柄、触控板、语音命令、USB/AOA、串口等），并在简介与“许可证与出处致谢”小节明确标注基于上游 `NuoRan/GameScrcpy`（Apache-2.0）。
    - `LICENSING.md`：项目名改为 **anmacgamer**（`wyb-070110/anmacgammer`，维护者 `@wyb-070110`），保留上游 `NuoRan/GameScrcpy`（Apache-2.0）与 macOS 新增代码（GPL-3.0-or-later）的双层许可说明。
  - 清理原作者联系方式、旧仓库链接与 Windows 痕迹：
    - `CODE_OF_CONDUCT.md`：删除 `QQ 交流群 762961615`（中文导语与英文 Enforcement 两处），改为通过本仓库 [GitHub Security 页面](https://github.com/wyb-070110/anmacgammer/security/advisories/new)私下报告或联系维护者 `@wyb-070110`。
    - `SECURITY.md`：安全漏洞私下报告链接改为 `https://github.com/wyb-070110/anmacgammer/security/advisories/new`（及维护者 `@wyb-070110`），“电脑端程序 / PC app”改为“Mac 端程序 / Mac app”。
    - `CONTRIBUTING.md`、`CONTRIBUTING.en.md`：标题改为 **参与 anmacgamer 开发 / Contributing to anmacgamer**，Issues 链接指向 `wyb-070110/anmacgammer/issues`，将原有的 Windows 10/11、MSVC、`dev-env.ps1`、`msvc-debug`、`package.ps1`、Inno Setup 等全部重写为 macOS 开发环境、`mac-app`/`mac-release`/`posix-core` 构建与测试流程、`packaging/macos/package.sh` 打包说明及 GPL-3.0+ 贡献许可说明。
    - `.github/ISSUE_TEMPLATE/bug_report.yml`、`.github/ISSUE_TEMPLATE/config.yml`：替换所有 `NuoRan/GameScrcpy` 链接为 `wyb-070110/anmacgammer`，版本号字段改为 `anmacgamer 版本 / Version`，安装方式改为 `.dmg 安装镜像`，系统环境由 `Windows 版本` 改为 `macOS 版本与芯片型号`。
    - `AGENTS.md`、`docs/README.md`、`docs/architecture.md`、`docs/macos-port.md`、`docs/extensions/logic.{md,en.md}`、`src/app/help/{zh_CN,en}/touch-box.md`、`CHANGELOG.md`、`packaging/macos/RELEASE-NOTES-beta{1,2}.md`：项目名称统一为 `anmacgamer`，移除残余 `NuoRan` 链接、`WinUSB` 标题与 Windows 专属架构/命令描述。
- **状态**：**可合并**


---

### 4. 重新打包通用发布版 `.dmg`（含新应用图标与卷图标）

- **改动文件**：
  - `packaging/macos/package.sh`：构建 `.dmg` 时同步写入 `.VolumeIcon.icns` 并通过 `SetFile -a C` 启用自定义磁盘卷图标，使挂载后的 DMG 卷与 `GameScrcpy.app` 一并展示新图标。
- **构建与打包产物**：
  - 预设：`mac-release`（`arm64` + `x86_64` 通用二进制，`LSMinimumSystemVersion = 14.0`，零外部 Homebrew/Qt 路径依赖，内置 `gs-server.apk`、`gs-companion.apk`、`adb`、`Assets.car` 与全尺寸 `GameScrcpy.icns`）。
  - 产物路径：`/Users/york/GameScrcpy-macos-antigravity/build/package/GameScrcpy-2.2.1-beta2-macos-universal.dmg`（及同目录 `GameScrcpy.app`，大小 `148M`，SHA-256：`f74398e1f87899a43d51fe10b8ce2c0d355d411380fdad66a22dc10a48a9eef1`）。
- **状态**：**可合并**


---

## [2026-11-25 晚间] 清理未提交 A8 副本、合并主线（`0c92d8a`）与完成 A9（响应 `SIGTERM`/`SIGINT`/`SIGHUP` 正常退出）

### 0. 清理与合并主线 `ccr-e32722c2-75w11u`

- 按 `antigravity-next.md` 丢弃本地未提交的 A8 工作副本（`git checkout -- src tests` 并移除未跟踪的 `toolbar-*.png`），快进合并主线至 `0c92d8a`（已包含 Claude 合并的 A8 改动 `88dad52`、方向 B 图标与文档更新 `af89b52` 及 `e7dc990`）。
- 合并后验证 `ctest --preset mac-app`：174/174 全部通过（含新增的 `gs_floating_toolbar_tests` 与 `gs_toolbar_reveal_tests`，2 个真机测试跳过）。

### 1. 完成 A9：收到 `SIGTERM`、`SIGINT`、`SIGHUP` 时正常走应用退出流程

- **复现与根因定位**：
  1. 在独立 `GS_DATA_DIR` 演示副本（`--safe-mode --demo-device tests/data/video/emulator_h264.media`）复现：向演示进程发送 `kill -TERM <demo_pid>` 后等待 2 秒，进程仍在运行，日志停留在 `hub window shown ...`，完全没有触发退出。
  2. **根因一（`SIGTERM` / `SIGINT` 被 SDL3 吞掉）**：`DeviceManager` 启动时创建的 `InputCapture`（`src/platform/src/mac/InputCapture.mm`）调用 `SDL_InitSubSystem(SDL_INIT_GAMEPAD)`（`AudioOutput.cpp` 亦调用 `SDL_InitSubSystem(SDL_INIT_AUDIO)`）。SDL3 在未设置 `SDL_HINT_NO_SIGNAL_HANDLERS` 时会隐式初始化事件子系统并安装 `_SDL_HandleSIG` 捕获 `SIGINT` 与 `SIGTERM`，仅将其转为 SDL 内部的 `SDL_EVENT_QUIT` 事件；而主程序运行在 Qt 事件循环且未消费 SDL 事件队列，导致 `SIGTERM` 和 `SIGINT` 被静默吞掉。
  3. **根因二（`SIGHUP` 及默认信号直接杀进程，不走 Qt 清理）**：未安装信号监听时，默认 POSIX 终止信号直接由内核终止进程，不会调用 `QCoreApplication::quit()`，导致 `aboutToQuit`（`ExtensionHost::finish()`）、`DeviceManager::closeWindows()` / `~DeviceManager()`（恢复手机输入法、停止手机端服务与伴侣连接）及 `~AppSettings()`（落盘配置）均不会执行。
- **改动文件**：
  - `src/platform/include/gs/platform/QuitSignal.h` & `src/platform/src/posix/QuitSignal.cpp`（新增，接入 `src/platform/Posix.cmake`，不影响 Windows）：
    - 创建带 `FD_CLOEXEC | O_NONBLOCK` 的自管道（使用 `pipe` + `fcntl`，兼容 macOS 14.0+，不使用 macOS 27 专属的 `pipe2`），解除当前线程可能继承的 `SIGTERM`/`SIGINT`/`SIGHUP` 阻塞掩码，并通过 `sigaction`（`SA_RESTART`）注册异步信号安全处理函数（保留并恢复 `errno`）。
    - 首次收到信号时将信号编号写入自管道唤醒主循环；若退出过程中再次收到第 2 次信号，立即恢复 `SIG_DFL` 解除掩码并 `raise` 当场终止，避免退出卡死；析构时还原先前的 `sigaction`（且因未设为 `SIG_IGN`，`gs-scripthost` / `gs-exthost` / `adb` 等 `exec` 子进程仍保持正常默认信号行为）。
  - `src/platform/src/mac/InputCapture.mm` & `src/media/src/posix/AudioOutput.cpp`：在 `SDL_InitSubSystem` 前设置 `SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1")`，禁止 SDL3 抢占 `SIGINT`/`SIGTERM` 处理器。
  - `src/app/main.cpp`（仅 `#if !defined(_WIN32)` 分支）：构造 `gs::platform::QuitSignal` 并挂接 `QSocketNotifier`，在 Qt 主线程收到信号后记录 `received SIGTERM/SIGINT/SIGHUP, quitting` 并调用 `QCoreApplication::quit()`（与菜单栏图标“退出”完全同一路径）。
  - `tests/unit/platform/posix/PosixPlatformTests.cpp`：新增 2 项 `fork` 子进程单元测试：
    1. 验证 `SIGTERM`、`SIGINT`、`SIGHUP`（即使预先被信号掩码阻塞，且同时启动了加载 SDL3 的 `InputCapture`）均能唤醒 `QuitSignal` 自管道并读出正确信号；
    2. 验证退出途中收到第二次 `SIGTERM` 时立即按默认信号终止而不阻塞。
- **验证命令与结果**：
  - `python3 tools/check_source_rules.py`：所改/新增文件 0 违规。
  - `ctest --preset mac-app -R gs_platform_tests -V`：41/41 用例（1025 断言）全部通过。
  - 独立 `GS_DATA_DIR` 演示副本实测（仅向自身演示副本 PID 发信号，未触碰任何其他进程）：
    - `SIGTERM`：`21:16:49.540 received SIGTERM, quitting` → `21:16:49.666 GameScrcpy exits with code 0`（耗时 ~126 ms，退出码 `0`）；
    - `SIGINT`：`21:16:51.634 received SIGINT, quitting` → `21:16:51.739 GameScrcpy exits with code 0`（耗时 ~105 ms，退出码 `0`）；
    - `SIGHUP`：`21:16:53.462 received SIGHUP, quitting` → `21:16:53.570 GameScrcpy exits with code 0`（耗时 ~108 ms，退出码 `0`）。
  - `ctest --preset mac-app -j8`：174/174 全部通过（2 个真机测试跳过）。
- **状态**：**可合并**


---

## [2026-11-26 晚间] 合并主线（`32b3213`）与完成 A10（无 adb 投屏：单次扫码持久化配对、首条投屏连接不掉线、停止投屏立即断开）

### 1. 根因定位（对照 `gamescrcpy.log` 21:09–21:10、21:44:49、21:55:59）

- **Bug 1（反复扫码 & 扫码后首条 `GSCM` 投屏连接 2 ms 内断开）**：
  1. **旧配对抢占新扫码的 `GSCM` 握手（21:44:49.477→49.489、21:10:50）**：Mac 已有旧配对时重新扫码，`CompanionLan::serve` 仅在 `!expectation.paired` 为真时才更新 `m_expected`，`DeviceManager::syncLanService` 也只在 `!expectation.paired` 时才用 `row.lanPairing`（`devices.json` 保存走 `Qt::QueuedConnection` 尚未落盘），且 `GSCL` 的 `provenWelcome` 与伴侣并行的 `mirrorOver` 存在竞态。结果紧随其后的 `GSCM` 投屏连接拿到的是**旧** `pc`（`34c0e5…`），手机端 `MirrorConnection.join` 校验 `reply.pc != 04413604…` 立即关掉套接字；Mac 侧 `switchToCompanionPicture → connectDevice` 发现 `companionMirrorReady` 已假，回退 `connectAoa`（`Connecting` 阶段又使 `syncCompanionLan` 误判 `!wanted` 杀掉刚连上的 `CompanionLink` 与 `CompanionLan`）。
  2. **端口跳变与旧配对列表冲突（解释 21:09–21:10 成对出现的 `did not prove it holds the key`，与 `6be9581` 签名变更无关，Keychain 解封正常）**：`CompanionLan` 绑定默认端口 `27185` 未设 `SO_REUSEADDR`，重启监听时因 `TIME_WAIT` 退回随机临时端口（`59104`/`62359`/`54714`）；每次重扫又生成新 `pc`，手机 `PairingStore` 累积多条同机旧记录（端口 `27185`）。Mac 重启回 `27185` 后，伴侣先试最新配对的旧临时端口失败，再试两条旧配对连上 `27185`，收到 `reply.pc = 81607b39…` 与旧配对预期不符即直接 `socket.close()`（Mac 读 `CompanionWelcome` 得 EOF 打出两条间隔 ~15 ms 的 `turned away`），却未用已知列表里匹配的 `81607b39…` 立即连 `27185`。
  3. **21:44:38 未自动回连**：21:15–21:43 走有线 adb 安装并启动伴侣 2.3.0（仅 `EXTRA_TOKEN` 本地 socket，`lanServer == null`），21:43:56 adb 会话结束时 `ServerEvents.ended` 直接 `stopSelfResult` 停掉服务；随后打开伴侣 `MainActivity` 时也未在 `autoConnect` 开启且有配对时主动触发 `ACTION_CONNECT`。
- **Bug 2（投屏停止后不会自动断开，21:55:59→21:56:14）**：
  1. 用户在手机状态栏点“停止共享”触发 `MirrorServer.projectionStopped()` 时，仅调用 `session?.control?.close()` 未发 `ServerGoodbye(None)`（且 `MirrorWriter.op(CLOSE)` 原先会清空 `urgent` 队列中排在 `CLOSE` 前的控制帧）；Mac `Session` 将通道 EOF 当作掉线进入 15 秒 `Reconnecting` 并立即重开通道 `0`，反过来让伴侣再次弹出录屏授权窗，最后以 `Failed` 报错收场。
  2. 用户在伴侣或通知栏点“停止”（`CompanionService.stopByUser`）时，`connector?.close()` 先于 `remote?.stop()` 直接关掉底层 TCP 套接字，`GSCL` 与 `GSCM` 的 `ServerGoodbye(None)` 均发不出去；且 `DeviceManager::companionNotice` 在 `companionPictured(row)` 为真时未处理 `StoppedOnPhone`（掉线重连也误走 adb 的单次 `companionRetried` 分支）。

### 2. 改动（代码提交 `10715fe`）

- **Mac（`src/app/CompanionLan.{h,cpp}`, `src/app/DeviceManager.cpp`, `src/session/src/Session.cpp`）**：
  - `CompanionLan`：监听套接字开启 `SO_REUSEADDR` 稳定复用 `27185`；同机重扫复用已有 `pc`，用 `m_pairingInFlight` 让并行的 `GSCM` 等完 `provenWelcome`，并无条件覆盖旧 `expectation.paired`、清空一次性 `token`。
  - `DeviceManager`：`syncLanService` 在 `row.lanPairing` 存在时优先采用新配对（落盘后清除）；`AoaSession` `Connecting` 过渡期不断开已连上的伴侣，会话结束（`!wanted`）同步 `detach` 并释放 `companionMirror`；`companionPictured(row)` 收到 `StoppedOnPhone` 立即 `disconnectDevice`，收到 `Lost` 每次均重走 `syncCompanionLan` 重新广播 mDNS。
  - `Session`：控制通道收到 `ServerGoodbye(ErrorCode::None)` 时记日志并直接结束流循环进入 `SessionPhase::Stopped`（不进 15 秒重连、不报错，窗口干净关闭、设备卡片回到未投屏状态）。
- **Android 伴侣（`android/companion/`，`versionCode = 9` / `2.3.1`）**：
  - `MirrorServer.kt`：`MirrorWriter` 在 `CLOSE` 时保留此前入队的控制帧并支持 `flush()`；`projectionStopped()` 与新增 `MirrorServer.stop()` 先发 `ServerGoodbye(ErrorCode.NONE)` 再关通道与释放录屏。
  - `LanConnector.kt`：重连在某端口收到 `OtherPc` 时若匹配 `PairingStore` 中已有电脑，立即以对应密钥连该端口并更新地址；`start`/`nudge` 重置退避间隔；新增 `stop()` 仅停重连循环、不提前掐断正在发 `ServerGoodbye` 的连接。
  - `SettingsSync.kt`、`CompanionService.kt`、`MainActivity.kt`：同一 `computer` ID 更换 `pc` 时自动清理 `PairingStore` 旧 `pc`；`stopByUser` 按 `connector.stop() → mirror/local/remote.stop() → connector.close()` 顺序优雅退出；adb 本地会话结束及首次打开 `MainActivity` 时若 `autoConnect` 开启且有配对记录，自动拉起局域网重连。
- **测试（`tests/unit/app/CompanionLanMacTests.cpp`, `tests/unit/session/CompanionMirrorTests.cpp`, `tests/PortProbe.cmake`, `LanConnectorTest.kt`, `SettingsSyncTest.kt`, `MirrorServerTest.kt`）**：
  - 新增 `gs_companion_lan_tests`（覆盖已有旧配对下重新扫码并立即并行发起 `GSCM` 投屏连接、重启监听复用原端口）与 `CompanionMirrorTests` 干净停止用例；伴侣侧补充对应 JUnit 用例。

### 3. 验证

- `python3 tools/check_source_rules.py`：所改文件 0 违规。
- `cd android && JAVA_HOME=/opt/homebrew/opt/openjdk@17 ANDROID_HOME=~/Library/Android/sdk ./gradlew :server:testDebugUnitTest :server:assembleRelease :companion:assembleDebug --offline`：`BUILD SUCCESSFUL`（新伴侣 2.3.1 已打入 `build/mac-app/bin/anmacgamer.app/Contents/Resources/gs-companion.apk`）。
- `cmake --build --preset mac-app -j8` & `ctest --preset mac-app -j8`：**176/176 全部通过**（含新增 `gs_companion_lan_tests`，2 个真机测试跳过）。

- **状态**：**可合并**（未触碰手机与运行中的用户实例；实机测试前需将最新伴侣 2.3.1 `gs-companion.apk` 装到手机上）。

