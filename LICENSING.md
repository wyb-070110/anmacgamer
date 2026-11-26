# Licensing

**anmacgamer** ([wyb-070110/anmacgamer](https://github.com/wyb-070110/anmacgamer)), maintained by [@wyb-070110](https://github.com/wyb-070110), is built upon [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy) and contains two categories of code:

| Part | License | How to tell |
|:--|:--|:--|
| Code kept from upstream ([NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy), including upstream files modified for macOS) | [Apache License 2.0](LICENSES/Apache-2.0.txt) (same text as [LICENSE](LICENSE)) | Files without an SPDX tag |
| Code written for anmacgamer on macOS (files added after 2026-11-16: macOS platform layer, POSIX ports, shaders, packaging and build scripts, tests) | [GNU GPL 3.0 or later](LICENSES/GPL-3.0-or-later.txt) | Files starting with `SPDX-License-Identifier: GPL-3.0-or-later` |

GPL 3.0 is compatible with Apache 2.0, so both compile together. **The distributed macOS application (`.dmg`) is offered as a whole under GPL 3.0 or later**: anyone distributing it (as is or modified) must provide the corresponding source code under the same terms. Upstream files keep their Apache 2.0 license and notices.

Third-party components keep their own licenses (listed in the app under **Settings > About > Open-source licenses** and in [third_party/README.md](third_party/README.md)). Libraries compiled for and shipped with the macOS build:

| Component | Version | License |
|:--|:--|:--|
| FFmpeg (`avcodec`, `avformat`, `avutil`, `swresample`; h264/hevc/av1/opus/aac decoders and mp4 muxer only, **no GPL parts**) | 9.0.2 | LGPL 2.1 or later |
| dav1d (statically linked into `avcodec`) | 1.5.4 | BSD 2-Clause |
| SDL3 | 3.4.18 | zlib |
| OpenSSL | 3.5.9 | Apache 2.0 |
| libusb | 1.0.30 | LGPL 2.1 or later |
| Qt | 6.10.3 | LGPL 3.0 |
| adb (Android SDK Platform-Tools) | — | Apache 2.0 |

The LGPL libraries are shipped as separate dynamic libraries in `anmacgamer.app/Contents/Frameworks` and can be replaced; their sources and build steps are in `tools/build_macos_deps.sh`.

---

## 简体中文

**anmacgamer**（[wyb-070110/anmacgamer](https://github.com/wyb-070110/anmacgamer)）由 [@wyb-070110](https://github.com/wyb-070110) 维护，基于上游项目 [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy) 移植并开发，包含两部分代码：

- 沿用的上游代码（来自 [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy)，含为 macOS 修改过的上游文件）：[Apache License 2.0](LICENSES/Apache-2.0.txt)（原文同 [LICENSE](LICENSE)），无 SPDX 标记。
- anmacgamer 为 macOS 新写的代码（2026-11-16 之后新增的 macOS 平台层、POSIX 移植、着色器、打包与构建脚本、测试等）：[GNU GPL 3.0 或更高版本](LICENSES/GPL-3.0-or-later.txt)，文件开头注明 `SPDX-License-Identifier: GPL-3.0-or-later`。

GPL 3.0 与 Apache 2.0 兼容。**发布的 macOS 程序（`.dmg`）整体按 GPL 3.0 或更高版本提供**：任何人原样或修改后分发，都必须以相同条款提供对应源码。沿用的上游文件保留 Apache 2.0 及其版权声明。随程序分发的自编译库见上表；LGPL 库以动态库形式放在 `anmacgamer.app/Contents/Frameworks` 中，可自行替换，源码与构建方式见 `tools/build_macos_deps.sh`。
