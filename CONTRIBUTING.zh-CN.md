# 参与 anmacgamer 开发

[English](CONTRIBUTING.md) · 简体中文

## 报告问题

- 先在 [Issues](https://github.com/wyb-070110/anmacgamer/issues) 搜索是否已有相同问题。
- 写明程序版本、macOS 版本与芯片、手机型号与 Android 版本、连接方式、复现步骤，并附上**设置 > 关于 > 导出诊断包**。
- 安全问题请按 [SECURITY.md](SECURITY.md) 私下提交。

## 准备环境并构建

要求：macOS 14+（Apple 芯片或 Intel）、Xcode 16+（重编译 `.icon` 需 Xcode 26+）、Qt 6.10.3（默认路径 `~/Qt/6.10.3/macos`）、CMake 3.28+、Ninja、Python 3.10+。修改 Android 端需 JDK 17 与 Android SDK platforms 35/36。

```bash
brew install cmake ninja openssl@3 ffmpeg sdl3 libusb android-platform-tools
cmake --preset mac-app
cmake --build --preset mac-app
ctest --preset mac-app
```

CMake 配置必须离线完成（不要引入 `FetchContent` 等构建期下载）。Qt 在其他路径时传 `-DCMAKE_PREFIX_PATH=<路径>`。产物输出在 `build/<预设>/bin`。

| 预设 | 用途 |
|:--|:--|
| `mac-app` | 本地开发构建（`anmacgamer.app`、`gs` 与测试） |
| `mac-release` | 面向 macOS 14+ 的通用版（`arm64` + `x86_64`）发布构建 |
| `posix-core` | 无 Qt 核心库与测试（Debug） |
| `posix-core-release` | 优化核心库与基准测试 |

Android 端（`server` 必须保持纯 Java；依赖版本在 `android/gradle/libs.versions.toml`）：

```bash
cd android && ./gradlew build
```

## 提交 PR 前的检查

- 单元测试：`ctest --preset mac-app`（可用 `-R <正则>` 筛选）。真机测试仅在设置 `GS_ADB_TEST_SERIAL` 时运行。
- QML 静态检查（要求 0 警告）：`cmake --build --preset mac-app --target all_qmllint`。
- 源码规则：`python3 tools/check_source_rules.py`（禁止解析 `dumpsys`/`wm` [R5]；禁止用 `sleep`/`postDelayed` 等固定延时做同步，除非标注 `source-rule-allow R9: <理由>`；检查英文源文与译文、`src/ui/controls` 仅用主题值、以及 QML `Text` 的 `textFormat`）。
- 无手机调试窗口：`./build/mac-app/bin/anmacgamer.app/Contents/MacOS/anmacgamer --demo-device tests/data/video/emulator_h265.media`（可用 `GS_DATA_DIR` 隔离配置）。

## 仓库结构

| 路径 | 内容 |
|:--|:--|
| `src/` | macOS 端（C++20 / ObjC++20、Qt 6 / Metal）与 `gs` 命令行；见 [docs/architecture.md](docs/architecture.md) |
| `android/` | `protocol`、`server`（Java）、`companion`（Kotlin） |
| `protocol/` | `SPEC.md` 与两端共用的金标准测试向量 |
| `sdk/` | 生成的脚本接口文档/类型与原生扩展 C 头文件 |
| `tests/`、`examples/`、`tools/`、`packaging/macos/`、`third_party/` | 测试、示例扩展、构建与检查脚本、`.dmg` 打包、第三方依赖 |

## 专项修改流程

- 修改协议：编辑 `protocol/SPEC.md`，更新 `protocol/tools/gen_vectors.py` 并运行 `python3 protocol/tools/gen_vectors.py`，确保 C++ 与 Java 测试都通过。不要手改生成的向量文件。
- 修改脚本接口：改完 `src/script/src/Api.cpp` 后，在仓库根目录运行 `./build/mac-app/bin/gs api write sdk`。
- 界面文字与帮助：代码内用英文写 `tr()`/`qsTr()`，运行 `anmacgamer_lupdate` 并补齐 `src/app/i18n/gamescrcpy_zh_CN.ts`。内置帮助在 `src/app/help/`（`index.json`、`en/`、`zh_CN/`，新增文件需加入 `src/app/CMakeLists.txt`）。
- 打包发布：运行 `tools/build_macos_deps.sh`，构建 `mac-release`，再运行 `packaging/macos/package.sh`。图标改动后在 Xcode 26+ 下运行 `packaging/macos/icon/build_icon.sh`。

## 提交 Pull Request

1. Fork 仓库并从 `main` 拉出分支。较大功能、协议或方案格式改动请先开 Issue 讨论。
2. 每个 PR 只做一件事并附带测试（修 bug 先写失败测试）。
3. 按 `.clang-format` 与 `.editorconfig` 格式化代码；界面样式取自 `src/ui/theme`；尽量少写行内注释。
4. 提交说明用英文（首行不超过 72 字符），并在 PR 中写明改动与验证方式。

## 许可证

提交新文件或 macOS 移植代码即同意以 [GNU GPL 3.0 或更高版本](LICENSES/GPL-3.0-or-later.txt) 发布（新文件开头加 `SPDX-License-Identifier: GPL-3.0-or-later`）；对上游文件的修改遵循其原有许可证，见 [LICENSING.md](LICENSING.md)。参与社区请遵守[行为准则](CODE_OF_CONDUCT.md)。
