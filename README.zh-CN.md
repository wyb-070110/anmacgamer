<div align="center">

<img src="packaging/macos/icon/GameScrcpy-1024.png" width="96" height="96" alt="anmacgamer">

# anmacgamer

在 macOS 上投屏安卓手机，把键盘、鼠标、手柄映射成触控操作，并支持用蓝图或 TypeScript 写自动化脚本。

[![Releases](https://img.shields.io/github/v/release/wyb-070110/anmacgamer?include_prereleases&style=flat-square&label=release)](https://github.com/wyb-070110/anmacgamer/releases)
[![License](https://img.shields.io/badge/license-GPL--3.0%2B%20%2F%20Apache--2.0-blue?style=flat-square)](LICENSING.md)
[![Platform](https://img.shields.io/badge/macOS%2014%2B-Apple%20Silicon%20%26%20Intel-000000?style=flat-square)](#系统要求)

[English](README.md) · 简体中文

[下载发布版](https://github.com/wyb-070110/anmacgamer/releases) · [文档](docs/README.md) · [macOS 移植说明](docs/macos-port.md) · [更新日志](CHANGELOG.md)

<img src="docs/images/keymap-editor.png" width="860" alt="在投屏画面上摆放键位">

</div>

anmacgamer 是 [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy)（Apache-2.0）的原生 macOS 移植版，由 [@wyb-070110](https://github.com/wyb-070110) 维护，投屏思路源自 [scrcpy](https://github.com/Genymobile/scrcpy)。

## 系统要求

- 电脑：macOS 14 Sonoma 及以上（Apple 芯片或 Intel，通用版）
- 手机：Android 9 及以上（声音转发与无线调试配对需 Android 11+）
- 连接：USB 数据线，或与 Mac 在同一局域网的 Wi-Fi / 有线网络

## 安装

1. 从 [Releases](https://github.com/wyb-070110/anmacgamer/releases) 下载 `.dmg`，将 `anmacgamer.app` 拖入“应用程序”。
2. 测试版未签名，首次打开请右键选择**打开**。若系统提示“已损坏”，在终端运行一次：
   ```bash
   xattr -dr com.apple.quarantine /Applications/anmacgamer.app
   ```
3. 在手机开启开发者选项与 USB 调试（或无线调试），连接后在设备页点击**投屏**。按 `F1` 打开内置帮助。

## 已实现的功能

- 画面与声音：H.264 / H.265 硬解、AV1 解码、Metal 渲染，以及 Android 11+ 手机声音播放。
- 连接方式：USB（自带 `adb` 与 AOA v2 HID 触控）、无线调试（配对码或扫码）、Wi-Fi 直连（不通时自动回退 adb），以及通过 Bonjour 发现的 **anmacgamer Companion**。不开 USB 调试时，伴侣经 Wi-Fi 传回画面和声音，触控走数据线（AOA）。
- 输入控制：键鼠映射、双指触控板滑动、Xbox/PS4/PS5 手柄（含震动）、经伴侣输入法用 Mac 键盘直输中英文，以及内置《崩坏：星穹铁道》PC 键位方案。
- 窗口与工具：独立投屏与多机宫格、MP4 录屏、息屏投屏、文件与应用管理、虚拟屏幕启动应用、延迟测量。
- 自动化与扩展：蓝图与 TypeScript 脚本（含找图找色、OCR、语音、定时任务）、`gs` 命令行、本地 MCP 服务、AI 助手、`.gsext`/`.gsbundle` 扩展、原生 `.dylib` 扩展、串口外设与 `gamescrcpy://` 链接。

## 暂未支持

- 伴侣投屏走数据线（目前画面和声音走 Wi-Fi）
- HDMI 采集卡与低功耗蓝牙（BLE）外设
- 扩展自定义着色器画面滤镜（HLSL 转 Metal）
- Intel Mac 编辑器内的 TypeScript 类型检查（脚本可正常运行）
- Apple Developer ID 公证（测试版 `.dmg` 未签名）

当前进展见 [docs/macos-port.md](docs/macos-port.md)。

## 从源码构建

安装依赖，并安装 Qt 6.10.3 到 `~/Qt/6.10.3/macos`（装在别处时传 `-DCMAKE_PREFIX_PATH=<路径>`）：

```bash
xcode-select --install
brew install cmake ninja openssl@3 ffmpeg sdl3 libusb android-platform-tools
```

配置、编译并运行测试：

```bash
cmake --preset mac-app
cmake --build --preset mac-app
ctest --preset mac-app
```

不连手机体验演示画面：

```bash
./build/mac-app/bin/anmacgamer.app/Contents/MacOS/anmacgamer --demo-device tests/data/video/emulator_h265.media
```

只构建无 Qt 核心库可运行 `cmake --workflow --preset posix-core`。通用版 `.dmg`（`mac-release`）打包见 `tools/build_macos_deps.sh` 和 `packaging/macos/package.sh`。应用数据保存在 `~/Library/Application Support/anmacgamer`。

## 文档

- [使用帮助](docs/README.md)（与程序内 `F1` 相同）
- [macOS 移植说明](docs/macos-port.md) · [架构概览](docs/architecture.md)
- [脚本接口参考](sdk/docs/README.md) · [扩展开发指南](docs/extensions/README.zh-CN.md)
- [通信协议规范](protocol/SPEC.md) · [参与开发](CONTRIBUTING.zh-CN.md)

## 反馈与安全报告

- 问题与建议：[GitHub Issues](https://github.com/wyb-070110/anmacgamer/issues)（报 bug 请附上**设置 > 关于 > 导出诊断包**）。
- 安全漏洞：请按 [SECURITY.md](SECURITY.md) 通过 [GitHub Security 页面](https://github.com/wyb-070110/anmacgamer/security/advisories/new)私下提交。

## 许可证与致谢

anmacgamer 为 macOS 新编写的代码以 [GNU GPL 3.0 或更高版本](LICENSES/GPL-3.0-or-later.txt) 发布，沿用的上游代码保留 [Apache License 2.0](LICENSE)；发布的 macOS 程序整体按 **GPL 3.0 或更高版本**提供。详见 [LICENSING.md](LICENSING.md)。

- 上游出处：基于 [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy)（Apache License 2.0）移植与开发，感谢原作者。
- [scrcpy](https://github.com/Genymobile/scrcpy) 与 [QtScrcpy](https://github.com/barry-ran/QtScrcpy)。
- 第三方组件与许可证：[third_party/README.md](third_party/README.md)、[LICENSING.md](LICENSING.md)，以及程序内**设置 > 关于 > 开源许可**。
