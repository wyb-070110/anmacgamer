# anmacgamer 2.2.1 beta 1

**测试版**：第一个 macOS 测试版，欢迎试用并反馈问题。 / **Beta**: the first macOS test build — please report what you find.

文件 / File: `GameScrcpy-2.2.1-beta1-macos-arm64.dmg`

---

## 中文

### 系统要求
- Apple 芯片的 Mac（M1 及以后）。Intel Mac 暂不支持。
- macOS 27.2 或更新（本测试版附带的库按 macOS 27.2 构建；后续版本会降低要求）。
- 安卓 7 及以上的手机，开启 USB 调试（或安卓 11 以上的无线调试）。不需要另装 adb，程序里已自带。

### 安装
1. 打开 `.dmg`，把 GameScrcpy 拖进“应用程序”。
2. 第一次打开：本测试版没有 Apple 开发者签名，请在“应用程序”里**右键 GameScrcpy → 打开**，再点“打开”。
   如果提示“已损坏”，在“终端”运行一次：`xattr -dr com.apple.quarantine /Applications/GameScrcpy.app`
3. 第一次使用无线调试时允许“本地网络”；第一次投屏时手机上会自动装上 GameScrcpy Companion（用来打字）。

### 有什么
- **低延迟投屏**：显卡解码，画面按屏幕刷新均匀显示；H.264、H.265、AV1。
- **USB 与无线调试**（配对码 / 二维码），WiFi 直连或 adb 隧道。
- **键鼠映射与手柄**：不需要“输入监控”权限；Xbox、PS4、PS5 手柄与震动。
- **用 Mac 键盘打字**，中文也可以，手机键盘不会弹出。
- **触控板**：双指滑动操作手机；四种方式可选（翻页、自然甩动、跟手、滚轮），可调速度、惯性、单次上限。
- **投屏窗口**：工具栏可放在上下左右；缩放时保持手机比例；手机熄屏时可自动停止投屏；点画面可唤醒息屏（AOD）的手机。
- **macOS 原生界面**：Liquid Glass 风格、跟随系统深浅色、菜单栏（文件、显示、设备、窗口、帮助）、菜单栏图标与通知。
- 录屏与导出 MP4、方案与键位编辑、蓝图与 TypeScript 自动化、语音命令、AI 助手（密钥存在钥匙串）、扩展。

### 已知问题
- 触控板滑动的手感还在调整，欢迎告诉我们哪种方式最顺手。
- 不开 USB 调试的 AOA 触控、采集卡、串口/蓝牙外设、扩展画面滤镜暂不可用。
- 个别网络下“WiFi 直连”连不上：改用“adb 隧道”。
- 每次更新后 macOS 可能再次询问“本地网络”等权限（测试版没有固定签名）。

---

## English

### Requirements
- A Mac with Apple silicon (M1 or later). Intel Macs are not supported yet.
- macOS 27.2 or later (this beta's libraries are built for 27.2; later builds will lower this).
- An Android 7+ phone with USB debugging (or Android 11+ wireless debugging). adb is included.

### Install
1. Open the `.dmg` and drag GameScrcpy to Applications.
2. First launch: this beta is not signed with an Apple developer ID. In Applications, **right-click GameScrcpy →
   Open**, then Open. If macOS says it is damaged, run once in Terminal:
   `xattr -dr com.apple.quarantine /Applications/GameScrcpy.app`
3. Allow Local Network the first time you use wireless debugging; the GameScrcpy Companion app is installed on the
   phone the first time you mirror (it receives what you type).

### What's in it
- **Low-latency mirroring**: decoded on the graphics card, shown evenly on the screen's refresh; H.264, H.265, AV1.
- **USB and wireless debugging** (pairing code or QR), WiFi direct or adb tunnel.
- **Key mapping and gamepads**: no Input Monitoring permission needed; Xbox, PS4, PS5 controllers with rumble.
- **Type with the Mac keyboard**, Chinese included, without the phone's keyboard popping up.
- **Trackpad**: two-finger swipes drive the phone, in four modes (page turn, natural fling, direct, scroll wheel),
  with speed, inertia and a per-swipe limit.
- **Mirror window**: toolbar on any side; resizing keeps the phone's proportions; can stop mirroring when the phone's
  screen turns off; a click wakes a phone on its Always On Display.
- **Native macOS look**: Liquid Glass style, follows light/dark mode, a menu bar (File, View, Device, Window, Help),
  a menu bar item and notifications.
- Recording and MP4 export, profiles and key editing, Blueprints and TypeScript automation, voice commands, the AI
  assistant (keys in the Keychain), extensions.

### Known issues
- Trackpad swiping is still being tuned — tell us which mode feels best.
- AOA touch without USB debugging, capture cards, serial/Bluetooth devices and extension video filters are not
  available on macOS yet.
- WiFi direct may fail on some networks: use adb tunnel.
- macOS may ask again for Local Network and other permissions after each update (no stable signature yet).
