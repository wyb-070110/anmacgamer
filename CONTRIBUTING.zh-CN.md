# 参与 anmacgamer 开发

[English](CONTRIBUTING.md) · 简体中文

本文档说明如何报告问题、准备 macOS 开发环境、编译和测试，以及提交代码前的约定。

## 报告问题

- 先在 [Issues](https://github.com/wyb-070110/anmacgamer/issues) 搜索是否已有相同问题。
- 使用问题反馈模板，写明程序版本、macOS 版本与芯片型号（Apple 芯片或 Intel）、手机型号与 Android 版本、连接方式和复现步骤，并附上**设置 > 关于 > 导出诊断包**生成的压缩包。
- 安全问题不要公开提 Issue，请按 [SECURITY.md](SECURITY.md) 私下报告。

## 提交改动

1. Fork 仓库并从 `main` 拉出分支。
2. 每个 PR 只做一件事，并附带测试。
3. 本地构建并运行相关测试，QML 静态检查保持 0 警告。
4. 提交说明用英文，首行不超过 72 个字符，需要时空一行补充原因。
5. 发起 PR 时说明改了什么以及如何验证。

较大改动（新功能、协议变更、方案格式变更）请先开 Issue 讨论。

## 仓库结构

| 路径 | 内容 |
|:--|:--|
| `src/` | macOS 端：C++20 / Objective-C++20 + Qt 6 / Qt Quick（Metal）程序本体与 `gs` 命令行（`src/cli`）；模块见 [docs/architecture.md](docs/architecture.md) |
| `android/` | 手机端：`protocol`、`server`（纯 Java）、`companion`（Kotlin） |
| `protocol/` | 协议规范 `SPEC.md` 及两端共用的金标准测试向量 |
| `sdk/` | 由 `gs api write` 生成的脚本接口类型与文档，以及原生扩展 C 头文件 |
| `tests/` | 单元测试、集成测试、界面测试与测试数据 |
| `examples/` | 示例组件、任务与扩展 |
| `tools/` | 开发与发布脚本（`build_macos_deps.sh`、源码规则检查、帧率与触控轨迹分析、生成器） |
| `packaging/macos/` | `.dmg` 打包脚本、图标构建脚本与发布说明 |
| `third_party/` | 随仓库提供的第三方依赖，见 [third_party/README.md](third_party/README.md) |

## 开发环境

| 工具 | 版本 |
|:--|:--|
| 操作系统 | macOS 14 Sonoma 及以上（Apple 芯片或 Intel） |
| Xcode / CLT | Xcode 16+（重编译 `.icon` 液态玻璃图标需 Xcode 26+） |
| Qt | 6.10.3（macOS，含 Qt Quick、Qt Quick Controls 2、Qt Shader Tools、Qt Test） |
| CMake 与 Ninja | CMake 3.28+，Ninja |
| Homebrew 依赖 | `brew install cmake ninja openssl@3 ffmpeg sdl3 libusb android-platform-tools` |
| Python | 3.10+ |
| JDK 与 Android SDK（可选） | JDK 17，Android SDK platforms 35 / 36（仅修改 `android/` 时需要） |

CMake 配置阶段必须离线完成，不要引入 `FetchContent` 等构建期网络下载。

## 构建 macOS 端

默认在 `~/Qt/6.10.3/macos` 查找 Qt（装在别处时传 `-DCMAKE_PREFIX_PATH=<路径>`）：

```bash
cmake --preset mac-app
cmake --build --preset mac-app
ctest --preset mac-app
```

可用预设：

| 预设 | 用途 |
|:--|:--|
| `posix-core` | 仅构建和测试无 Qt 依赖的核心库（Debug） |
| `posix-core-release` | 核心库优化构建与计时基准测试 |
| `mac-app` | 完整本地开发构建（`anmacgamer.app`、`gs` 及测试程序） |
| `mac-release` | 面向 macOS 14+ 的通用二进制（`arm64` + `x86_64`）发布构建，链接 `tools/build_macos_deps.sh` 编译的 LGPL 依赖 |

产物输出在 `build/<预设>/bin`（`anmacgamer.app`、`gs`、`gs-scripthost`、`gs-exthost` 及各测试程序）。

## 构建 Android 端

设置 `ANDROID_HOME` 后运行：

```bash
cd android
./gradlew build
```

编译 `protocol`、`server`、`companion` 三个模块，并运行 JUnit 与 lint（lint 警告视为错误）。`server` 必须保持纯 Java，不引入 Kotlin。依赖版本统一在 `android/gradle/libs.versions.toml` 管理。

## 测试

- 各模块使用 doctest（`gs_base_tests`、`gs_protocol_tests`、`gs_input_tests`、`gs_session_tests`、`gs_media_tests`、`gs_script_tests`、`gs_platform_tests`、`gs_hardware_tests`、`gs_render_tests` 等），通过 `ctest --preset mac-app` 运行（可用 `-R <正则>` 筛选）。
- QML 静态检查必须为 0 警告：`cmake --build --preset mac-app --target all_qmllint`。
- 协议测试读取 `protocol/vectors/` 的全部金标准向量，C++ 与 Java 两端都必须逐字节通过。
- 真机测试（`gs_adb_device_tests`、`gs_session_device_tests`）仅在设置 `GS_ADB_TEST_SERIAL` 时运行，否则自动跳过。
- 无手机调试投屏窗口：`./build/mac-app/bin/anmacgamer.app/Contents/MacOS/anmacgamer --demo-device tests/data/video/emulator_h265.media`；设置 `GS_DATA_DIR` 可隔离测试配置目录。

## 修改协议

1. 修改 `protocol/SPEC.md`。
2. 更新 `protocol/tools/gen_vectors.py` 并运行 `python3 protocol/tools/gen_vectors.py`。
3. 确保 C++（`src/protocol`）和 Java（`android/protocol`）测试都通过。

不要手工修改生成的向量文件。

## 修改脚本接口

修改 `src/script/src/Api.cpp` 后，在仓库根目录运行 `./build/mac-app/bin/gs api write sdk` 重新生成 `sdk/`。

## 界面文字与翻译

- 代码中界面文字一律写英文（`tr()` / `qsTr()`），中文译文放在 `src/app/i18n/gamescrcpy_zh_CN.ts`。
- 增改文字后运行翻译更新目标（`anmacgamer_lupdate`）并补齐所有中文译文。
- 内置帮助在 `src/app/help/`：`index.json` 定义顺序、名称、图标和标题，`en/` 与 `zh_CN/` 放对应 Markdown，新增文章需加入 `src/app/CMakeLists.txt`。

## 源码规则

`python3 tools/check_source_rules.py` 会检查：

- 产品代码不运行、不解析 `dumpsys`、`wm` 等面向人的命令输出（R5）。
- 不用固定延时做同步：禁止 sleep、带延时的单发定时器、`postDelayed`（R9）。延时本身就是功能时，同行注明 `source-rule-allow R9: <理由>`。
- 面向用户的文字源文为英文且有对应译文。
- `src/ui/controls` 控件仅从主题取颜色、尺寸和时长。
- 每个 QML `Text` 声明 `textFormat`。

## 代码约定

- C++20 / Objective-C++20，遵循 `.clang-format` 与 `.editorconfig`，标识符与提交说明用英文。
- 尽量少写行内注释（文件头简短说明除外），解释放在 PR 或文档里。`source-rule-allow` 是唯一要求写在行内的说明。
- 界面颜色、字号、间距、圆角和动效统一取自 `src/ui/theme`。
- 新功能配测试；修 bug 先写失败测试。

## 打包发布

1. 运行 `tools/build_macos_deps.sh` 构建面向 macOS 14+ 的通用依赖（LGPL FFmpeg、dav1d、SDL3、OpenSSL、libusb）。
2. 构建 `mac-release` 预设。
3. 运行 `packaging/macos/package.sh` 打入 Qt 框架、动态库、`adb` 与签名，生成 `.dmg`。
4. 修改 `packaging/macos/icon/` 下的图层或配置后，在 Xcode 26+ 环境下运行 `packaging/macos/icon/build_icon.sh` 更新 `Assets.car` 与 `.icns`。

## 许可证

提交新文件或 macOS 移植代码即同意以 [GNU GPL 3.0 或更高版本](LICENSES/GPL-3.0-or-later.txt) 发布（新文件开头加 `SPDX-License-Identifier: GPL-3.0-or-later`）；对上游文件的修改遵循其原有许可证，见 [LICENSING.md](LICENSING.md)。参与社区请遵守[行为准则](CODE_OF_CONDUCT.md)。
