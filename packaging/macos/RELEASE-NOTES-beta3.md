# anmacgamer 2.2.1 beta 3

Third macOS beta build / 第三个 macOS 测试版。
File / 文件：`anmacgamer-2.2.1-beta3-macos-universal.dmg` (Apple silicon and Intel, universal / Apple 芯片与 Intel 通用)

## English

### What is new since beta 2

- Trackpad swipes: touches follow Mac input timing and lift-off velocity, so home screens flip one page at a time and lists keep native scroll momentum. Direct mode is now the default.
- Customisable toolbar: open **More > Toolbar buttons...** to hide buttons or pin notifications, quick settings, files, latency, and mini window. Side toolbars show all buttons.
- Screen-off mirroring: new switch in Settings turns off the phone screen while mirroring.
- Pinyin preview: Chinese pinyin being typed on Mac now shows on screen.
- New icon and name: Liquid Glass icon on macOS 26+ and a new DMG icon; project renamed to **anmacgamer**.
- Hardware: USB AOA touch and serial devices are now wired up on macOS (limited phone testing so far).

### Requirements and install

- macOS 14 Sonoma or later (Apple silicon or Intel); Android 9 or later (Android 11+ for wireless debugging and audio). `adb` is bundled.
- Open the `.dmg` and drag `anmacgamer.app` to Applications.
- Beta builds are unsigned. On first launch, right-click the app and choose **Open**. If macOS says it is damaged, run once in Terminal:
  ```bash
  xattr -dr com.apple.quarantine /Applications/anmacgamer.app
  ```
- Allow Local Network when using wireless debugging. **anmacgamer Companion** installs on the phone on first mirror to support Mac typing.

### Known issues

- AOA touch and serial devices are new; please report issues. Capture cards, Bluetooth LE devices, and extension video filters are not available yet.
- If Wi-Fi direct fails on your network, switch to the adb tunnel.
- macOS may re-prompt for permissions after updates because beta builds are unsigned.
- On Intel Macs, TypeScript scripts run normally, but the editor skips type checking.

## 中文

### 相比 beta 2 的变化

- 触控板滑动：手机按 Mac 的输入节拍与离手速度接收触控，主屏快甩只翻一页，列表保留系统惯性。默认模式改为“跟手”。
- 自定义工具栏：在**更多 > 工具栏按钮…**中隐藏按钮，或固定通知栏、快捷设置、文件、延迟测量和迷你窗口。侧边工具栏完整显示所有按钮。
- 息屏投屏：设置新增开关，投屏时可自动关闭手机屏幕。
- 拼音显示：用 Mac 打中文时，画面直接显示正在拼写的拼音。
- 新图标与更名：macOS 26+ 支持液态玻璃图标并更新了安装盘图标；项目更名为 **anmacgamer**。
- 硬件支持：已接入 USB AOA 触控与串口外设（真机覆盖尚有限）。

### 系统要求与安装

- macOS 14 Sonoma 及以上（Apple 芯片或 Intel）；Android 9 及以上（无线调试与声音转发需 Android 11+），程序自带 `adb`。
- 打开 `.dmg`，将 `anmacgamer.app` 拖入“应用程序”。
- 测试版未签名，首次打开请右键选择**打开**。若提示“已损坏”，在终端运行一次：
  ```bash
  xattr -dr com.apple.quarantine /Applications/anmacgamer.app
  ```
- 首次无线调试请允许“本地网络”。首次投屏时手机会自动安装 **anmacgamer Companion** 以支持 Mac 键盘打字。

### 已知问题

- AOA 触控与串口刚接入，遇到问题请反馈；采集卡、蓝牙外设与扩展画面滤镜暂未支持。
- 部分网络下 Wi-Fi 直连不通时，请改用 adb 隧道。
- 因测试版无固定签名，更新后 macOS 可能再次询问权限。
- Intel Mac 上 TypeScript 脚本正常运行，但编辑器不做类型检查。
