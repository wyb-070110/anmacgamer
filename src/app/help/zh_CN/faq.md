常见问题的快速排查方法。

## 连接 {icon=smartphone} {tone=azure}

### 设备页看不到手机 {#no-phone}

1. 确认使用的是支持数据传输的 USB 线（而非仅充电线）。
2. 在手机「开发者选项」中打开 **USB 调试**。
3. 将 USB 连接用途选为**文件传输**。
4. 解锁手机，在「允许 USB 调试吗？」弹窗中点**允许**。

> [!TIP]
> 需要按手机品牌查看步骤？[打开新手引导](action:guide)。

### 手机一直显示「等待授权」 {#waiting-authorization}

解锁手机，勾选「一律允许使用这台计算机进行调试」后点**允许**。

### 手机显示「离线」 {#offline}

重新插拔 USB 数据线；若仍显示离线，在开发者选项中关闭再重新打开 **USB 调试**。

### 提示 adb 不可用 {#adb-missing}

请安装 Android SDK Platform-Tools（例如 `brew install --cask android-platform-tools`），确认没有其他程序占用 adb，然后点击**设备**页的**重试**。

### 三星手机的 USB 调试开关是灰色的 {#samsung-grey}

请先在手机上关闭**设置 > 安全和隐私 > 自动拦截程序**。

## 画面与声音 {icon=monitor-play} {tone=orange}

### 没有声音 {#no-sound}

- 传音需要 Android 11 及以上系统（Android 11 连接前需先解锁手机）。
- 确认**设置 > 音频 > 声音**已开启，且投屏工具栏未静音。
- 注意：投屏期间音量由 Mac 控制，手机侧面的音量键不会改变声音大小。

### WiFi 直连连不上 {#wifi-direct}

确保 Mac 和手机在同一网络，并在**系统设置 > 隐私与安全性 > 本地网络**中允许 **anmacgamer**；检查路由器是否开启 AP 隔离或防火墙拦截了 UDP。仍不行时，在设备卡片上切回 **adb 隧道**。

### 画面卡顿或延迟高 {#lag}

- 优先使用 USB 数据线连接。
- 在**设置 > 投屏与画质**中适当调低**最大分辨率**或**码率上限**，或开启**低延迟模式**。
- 打开诊断信息查看帧率、延迟和 Metal 硬件加速状态（未启用 Metal 的虚拟机回退到 CPU 渲染会比较卡）。

### 中文或特殊字符打不进手机 {#typing}

非英文字符默认经手机剪贴板发送（除非使用[anmacgamer 伴侣](help:companion)输入法）。请检查手机是否限制了后台剪贴板访问，或启用伴侣输入法。

## 按键与操控 {icon=keyboard} {tone=lime}

### 有画面，但鼠标和键盘没反应 {#no-input}

部分品牌手机需要额外开启一个开发者开关，开启后重新连接：

| 品牌 | 需开启的开关 |
|---|---|
| 小米 · Redmi · POCO | **USB 调试（安全设置）**（需登录小米账号，开启后重启手机）。 |
| OPPO · 一加 · 真我 | **禁止权限监控**（ColorOS 16+ 需先把手机语言切为英文才会显示 **Disable system optimization**，开完再切回中文）。 |
| vivo · iQOO | **USB 模拟点击**。 |

### 方案里的按键没反应 {#keys-nothing}

- 确认投屏工具栏上的**键位映射**开关已打开。
- 在**键位映射选项**中确认已选中方案。
- 点击一下投屏窗口，确保它处于焦点状态。

### 鼠标指针不见了 {#pointer-gone}

当前处于视角锁定状态。再按一次视角锁键即可恢复（默认是 Tab 上方、1 左侧的按键；内置《崩坏：星穹铁道》方案为 <kbd>Tab</kbd>）。

### 快捷键没有反应 {#shortcut-nothing}

打开**设置 > 快捷键速查**检查是否冲突。投屏窗口内，当前方案绑定的按键优先于应用快捷键。

## 其他 {icon=settings-2} {tone=gray}

### anmacgamer 意外退出 {#crashed}

下次启动时点击**导出诊断信息…**（或随时在**设置 > 关于**导出），生成的 zip 包含日志和崩溃报告（序列号、IP、剪贴板内容等隐私信息已自动剔除）。

### 某个扩展导致异常 {#extension-trouble}

在**扩展**页使用**查找问题扩展**排查，或用 `open -a anmacgamer --args --safe-mode` 临时禁用全部扩展启动。详见[扩展 · 故障排查](help:extensions#trouble)。

### 设置和方案保存在哪里？ {#data-folder}

位于 `~/Library/Application Support/anmacgamer`（`profiles/` 与 `logs/`）。启动前设置 `GS_DATA_DIR` 环境变量可指定其他目录。

### 支持哪些手机？ {#which-phones}

Android 9 及以上系统。HarmonyOS 5 及以上不再基于 Android，无法投屏。

### 在哪里查看所有命令？ {#all-commands}

按 <kbd>⌘ Command</kbd>+<kbd>K</kbd> 打开命令面板，按 <kbd>F1</kbd> 打开帮助。
