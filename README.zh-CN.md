<div align="center">

<img src="packaging/macos/icon/GameScrcpy-1024.png" width="96" height="96" alt="anmacgamer">

# anmacgamer

macOS 安卓投屏、键鼠手柄映射与脚本自动化工具。

[![Releases](https://img.shields.io/github/v/release/wyb-070110/anmacgamer?include_prereleases&style=flat-square&label=release)](https://github.com/wyb-070110/anmacgamer/releases)
[![License](https://img.shields.io/badge/license-GPL--3.0%2B%20%2F%20Apache--2.0-blue?style=flat-square)](LICENSING.md)
[![Platform](https://img.shields.io/badge/macOS%2014%2B-Apple%20Silicon%20%26%20Intel-000000?style=flat-square)](#系统要求)

[English](README.md) · 简体中文

[下载发布版](https://github.com/wyb-070110/anmacgamer/releases) · [文档](docs/README.md) · [macOS 移植说明](docs/macos-port.md) · [更新日志](CHANGELOG.md)

<img src="docs/images/keymap-editor.png" width="860" alt="在投屏画面上摆放键位">

</div>

anmacgamer 是 [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy)（Apache-2.0）的原生 macOS 移植版，由 [@wyb-070110](https://github.com/wyb-070110) 维护，投屏思路源自 [scrcpy](https://github.com/Genymobile/scrcpy)。

它通过 USB 或 Wi-Fi 把安卓手机画面投到 Mac 上，将键盘、鼠标、触控板和手柄输入映射为触摸操作，并支持用蓝图或 TypeScript 编写自动化逻辑。界面自带简体中文与英文，可通过语言包扩展添加更多语言。

## 已实现的功能

- 画面与声音：VideoToolbox 硬解（H.264、H.265）与 dav1d 解码（AV1），通过 Qt RHI（Metal）按屏幕刷新节拍绘制。Android 11 及以上支持经 SDL3 播放手机声音（手机端固定音量采集，由 macOS 统一控制音量）。
- 连接方式：USB（自带 `adb`、libusb 热插拔、AOA v2 HID 触控）、无线调试（6 位配对码或扫码）、WiFi 直连（UDP + Reed-Solomon 纠删码，直连不通时自动回退 adb 隧道），以及手机伴侣应用（Bonjour 发现）。
- 输入控制：投屏窗口内键鼠映射（不需要“输入监控”权限）、内置《崩坏：星穹铁道》PC 键位方案、通过伴侣输入法用 Mac 键盘直输中英文（显示拼音组字，不弹手机软键盘）、SDL3 手柄支持（Xbox / PS4 / PS5 含震动）、双指触控板滑动（默认跟手，另提供翻页、自然甩动、滚轮模式）。
- macOS 集成：Liquid Glass 界面风格（跟随深浅色与降低透明度设置）、原生菜单栏、状态栏图标与通知、保持比例的独立投屏窗口与多机宫格、四边可自定义工具栏、跨桌面置顶、投屏时关闭手机屏幕、录屏导出 MP4、文件与应用管理、独立虚拟屏幕启动应用、端到端延迟测量。
- 自动化与扩展：蓝图与 TypeScript 共用同一份代码文件，运行在 `sandbox_init` 隔离的 `gs-scripthost` 进程中。支持模板匹配与找色、macOS Vision OCR、`AVSpeechSynthesizer` / `SFSpeechRecognizer` 语音朗读与命令识别、定时任务、`gs` 命令行工具、Unix 套接字本地 MCP 服务、AI 助手（密钥存 macOS 钥匙串）、`.gsext` / `.gsbundle` 扩展包、沙箱隔离的 `.dylib` 原生扩展（`gs-exthost`）、IOKit `/dev/cu.*` 串口外设与 `gamescrcpy://` 链接。

## 暂未支持

- 不开 adb、经伴侣 App 投屏（已做好，正在真机测试）
- HDMI 采集卡
- 低功耗蓝牙（BLE）外设
- 扩展自定义着色器画面滤镜（HLSL 转 Metal）
- Intel Mac 编辑器内的 TypeScript 类型检查（脚本可正常运行）
- Apple Developer ID 公证（测试版 `.dmg` 未签名）

当前进展与实测记录见 [docs/macos-port.md](docs/macos-port.md)。

## 系统要求

| 项目 | 要求 |
|:--|:--|
| 电脑 | macOS 14 Sonoma 及以上（Apple 芯片或 Intel） |
| 手机 | Android 9 及以上（声音转发与无线调试配对需 Android 11+） |
| 连接 | USB 数据线，或与 Mac 在同一局域网的 Wi-Fi / 有线网络 |

## 安装

1. 从 [Releases](https://github.com/wyb-070110/anmacgamer/releases) 下载通用版 `.dmg`，拖入“应用程序”。
2. 测试版未签名，首次打开请在“应用程序”中右键选择**打开**。若系统提示“已损坏”，在终端运行一次：
   ```bash
   xattr -dr com.apple.quarantine /Applications/anmacgamer.app
   ```
3. 在手机开启**开发者选项**和**USB 调试**（或**无线调试**），连接后在设备页点击**投屏**。按 `F1` 打开内置帮助。

## 从源码构建

安装依赖：

```bash
xcode-select --install
brew install cmake ninja openssl@3 ffmpeg sdl3 libusb android-platform-tools
```

安装 Qt 6.10.3（macOS）到 `~/Qt/6.10.3/macos`（若装在其他目录，配置时传 `-DCMAKE_PREFIX_PATH=<路径>`）。

编译并运行测试：

```bash
cmake --preset mac-app
cmake --build --preset mac-app
ctest --preset mac-app
```

不连手机体验演示画面：

```bash
./build/mac-app/bin/anmacgamer.app/Contents/MacOS/anmacgamer --demo-device tests/data/video/emulator_h265.media
```

只构建无 Qt 依赖的核心库：

```bash
cmake --workflow --preset posix-core
```

通用版 `.dmg`（`arm64` + `x86_64`，macOS 14+，自带 LGPL 依赖与 `adb`）的构建见 `tools/build_macos_deps.sh` 和 `packaging/macos/package.sh`。设置、方案、扩展与日志存放在 `~/Library/Application Support/anmacgamer`。

## 文档

| 内容 | 位置 |
|:--|:--|
| 使用帮助（与程序内 `F1` 相同） | [docs/README.md](docs/README.md) |
| macOS 移植说明与架构 | [docs/macos-port.md](docs/macos-port.md) · [docs/architecture.md](docs/architecture.md) |
| 脚本接口参考 | [sdk/docs/README.md](sdk/docs/README.md) |
| 扩展开发指南 | [docs/extensions/README.zh-CN.md](docs/extensions/README.zh-CN.md) |
| 通信协议规范 | [protocol/SPEC.md](protocol/SPEC.md) |
| 参与开发 | [CONTRIBUTING.zh-CN.md](CONTRIBUTING.zh-CN.md) |

## 反馈与安全报告

- 问题反馈与建议：[GitHub Issues](https://github.com/wyb-070110/anmacgamer/issues)。报 bug 时请附上**设置 > 关于 > 导出诊断包**生成的压缩包。
- 安全问题请勿公开提 Issue，按 [SECURITY.md](SECURITY.md) 通过 [GitHub Security 页面](https://github.com/wyb-070110/anmacgamer/security/advisories/new)私下报告。

## 许可证与致谢

anmacgamer 为 macOS 新编写的代码以 [GNU GPL 3.0 或更高版本](LICENSES/GPL-3.0-or-later.txt) 发布，沿用的上游代码保留 [Apache License 2.0](LICENSE)；发布的 macOS 程序整体按 **GPL 3.0 或更高版本**提供。详见 [LICENSING.md](LICENSING.md)。

- 上游出处：基于 [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy)（Apache License 2.0）移植与开发，感谢原作者。
- [scrcpy](https://github.com/Genymobile/scrcpy)：安卓低延迟投屏先驱，投屏思路源自该项目。
- [QtScrcpy](https://github.com/barry-ran/QtScrcpy)：GameScrcpy 1.x 的基础。
- 第三方开源组件（Qt、FFmpeg、dav1d、SDL3、OpenSSL、libusb、QuickJS-ng、TypeScript、QWindowKit、nlohmann/json、miniz、QR Code generator、Lucide、doctest、tl::expected）及其版本与许可证见 [third_party/README.md](third_party/README.md)、[LICENSING.md](LICENSING.md)，以及程序内**设置 > 关于 > 开源许可**。
