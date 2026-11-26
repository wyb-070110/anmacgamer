# anmacgamer 2.2.1 beta 3

Third macOS beta build / 第三个 macOS 测试版。
File / 文件：`anmacgamer-2.2.1-beta3-macos-universal.dmg` (Apple silicon and Intel, universal / Apple 芯片与 Intel 通用)

## English

### What is new since beta 2

- Mirroring without USB debugging: control the phone over the USB cable (AOA touch) while receiving picture and sound from the companion over Wi-Fi. Scan the QR code once per computer; if the phone was connected once with USB debugging, screen capture starts without Android's share-screen dialog.
- Companion 2.3: single Settings-style page in the Android 17 look with wallpaper colours; the phone stays quiet while the Mac plays its sound, and Mac volume is independent of the phone; Chinese pinyin typed on the Mac shows on screen without a keyboard bar; the previous phone keyboard returns on disconnect.
- Mirror window and toolbar: Keep-on-top button keeps the window above all apps and across desktops; the toolbar on any side always shows Back, Home, and Recents plus up to five chosen buttons (**More > Toolbar buttons...**); Settings adds a switch to turn off the phone screen while mirroring.
- Built-in Honkai: Star Rail profiles: phone and cloud versions with a PC key layout; the mouse stays free until `Tab` locks the view (`Option` shows the cursor while locked).
- Mac key names: key hints use `Option`, `Command`, `Control`, `Shift`, and `Delete`; function keys need `fn` unless "Use F1, F2, etc. keys as standard function keys" is turned on in macOS Keyboard settings.
- Trackpad and hardware: trackpad swipes follow Mac input timing and lift-off speed so home screens flip one page and lists keep scroll momentum (Direct mode is now the default); USB AOA touch and `/dev/cu.*` serial ports work on macOS.
- Wireless fallback: automatic transport falls back to the adb tunnel when the phone cannot be reached directly over Wi-Fi.
- Fixes: fixed a crash when a profile or task logic stopped; Disconnect now stops companion mirroring cleanly, and stopping capture on the phone closes the Mac session without retrying; the app quits cleanly on `SIGTERM`, `SIGINT`, and `SIGHUP`; the login Keychain is read once per launch instead of prompting repeatedly.
- Name and icon: renamed to **anmacgamer** across the app, DMG, and companion, with a Liquid Glass app icon and DMG volume icon; existing settings and profiles in `~/Library/Application Support/GameScrcpy` move over to `anmacgamer` once.

### Requirements and install

- macOS 14 Sonoma or later (Apple silicon or Intel); Android 9 or later (Android 11+ for wireless debugging and audio). `adb` is bundled.
- Open the `.dmg` and drag `anmacgamer.app` to Applications.
- Beta builds are ad-hoc signed. On first launch, right-click `anmacgamer` in Applications and choose **Open**. If macOS says it is damaged, run once in Terminal:
  ```bash
  xattr -dr com.apple.quarantine /Applications/anmacgamer.app
  ```
- Allow Local Network when using wireless debugging or companion Wi-Fi mirroring.

### Known issues

- The first launch of a new version asks once for the login Keychain password (choose **Always Allow**); macOS may also ask again for Local Network or other permissions after updates because beta builds have no Developer ID signature.
- **anmacgamer Companion** must be installed once over USB debugging (or wireless debugging) before mirroring without USB debugging.
- Mirroring through the companion uses Android screen capture, which shows a black screen in apps that block capture.
- Capture cards, Bluetooth LE devices, and extension video filters are not available on macOS yet; USB AOA touch and serial devices have had limited hardware testing.
- On Intel Macs, TypeScript scripts run as usual, but the editor skips type checking.

## 中文

### 相比 beta 2 的变化

- 免 USB 调试投屏：通过 USB 数据线发送触控（AOA），同时经 Wi-Fi 接收伴侣传来的画面与声音。每台电脑只需扫码配对一次；只要手机此前通过 USB 调试连过一次，后续开始投屏不再弹出系统的“共享屏幕”确认框。
- 伴侣 2.3：改为 Android 17 风格的单页设置界面并跟随壁纸取色；Mac 播放投屏声音时手机保持静音，且 Mac 端音量与手机独立；用 Mac 打中文时画面直接显示拼音且不占屏幕高度；断开连接后自动切回手机原来的输入法。
- 投屏窗口与工具栏：新增窗口置顶按钮，可在所有桌面空间保持最前；四边工具栏常驻返回、主屏、最近任务，并可在**更多 > 工具栏按钮…**中自选最多 5 个按钮；设置新增投屏时自动息屏开关。
- 内置《崩坏：星穹铁道》方案：含移动端与云·星穹铁道两套 PC 键位方案，鼠标默认自由移动，按 `Tab` 锁定视角（锁定时按住 `Option` 临时显示光标）。
- Mac 键名提示：键位提示统一显示 `Option`、`Command`、`Control`、`Shift`、`Delete`；未在系统键盘设置中开启“将 F1、F2 等键用作标准功能键”时，按功能键需配合 `fn`。
- 触控板与硬件：触控板滑动跟随 Mac 输入节拍与抬手速度，主屏快滑只翻一页，列表保留系统惯性（默认模式改为“跟手”）；接入 macOS USB AOA 触控与 `/dev/cu.*` 串口设备。
- 无线回退：“自动”连接在 Wi-Fi 直连不通时自动回退到 adb 隧道。
- 问题修复：修复逻辑停止时的崩溃；修复伴侣投屏点“断开”无效及手机端停止共享后不自动结束的问题；收到 `SIGTERM`、`SIGINT`、`SIGHUP` 时正常走退出清理；每次启动只读取一次登录钥匙串，不再反复弹窗。
- 更名与图标：应用、安装镜像与手机伴侣统一更名为 **anmacgamer**，换用液态玻璃图标与自定义安装盘图标；首次启动时自动将 `~/Library/Application Support/GameScrcpy` 中的旧配置与方案迁移到 `anmacgamer`。

### 系统要求与安装

- macOS 14 Sonoma 及以上（Apple 芯片或 Intel）；Android 9 及以上（无线调试与声音转发需 Android 11+），程序自带 `adb`。
- 打开 `.dmg`，将 `anmacgamer.app` 拖入“应用程序”。
- 测试版无开发者签名，首次打开请在“应用程序”中右键 `anmacgamer` 选择**打开**。若提示“已损坏”，在终端运行一次：
  ```bash
  xattr -dr com.apple.quarantine /Applications/anmacgamer.app
  ```
- 使用无线调试或伴侣 Wi-Fi 投屏时，请允许“本地网络”权限。

### 已知问题

- 更新新版本后首次启动会询问一次登录钥匙串密码（点**始终允许**即可）；由于测试版无固定开发者签名，macOS 也可能再次询问“本地网络”等权限。
- 免 USB 调试投屏前，需先开一次 USB 调试（或无线调试）将 **anmacgamer Companion** 安装到手机上。
- 经伴侣投屏使用系统录屏接口，禁止截屏录屏的应用会显示黑屏。
- 采集卡、低功耗蓝牙外设与扩展画面滤镜暂未支持；USB AOA 触控与串口设备真机覆盖仍有限。
- Intel Mac 上 TypeScript 脚本照常运行，但编辑器不做类型检查。
