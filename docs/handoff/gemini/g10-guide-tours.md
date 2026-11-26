# G10 — First-Run Guide & Page Tour Copy

> Rewritten copy for QML overlays (`GuideOverlay.qml`, `TourOverlay.qml`, and all `TourOverlay` step lists).  
> Format: File / Step | Old English | New English | New Chinese (`zh_CN`)

---

## 1. `src/app/qml/GuideOverlay.qml`

| Step / Element | Old English | New English | New Chinese |
|---|---|---|---|
| Page 0 · Title | Welcome to anmacgamer | Welcome to anmacgamer | 欢迎使用 anmacgamer |
| Page 0 · Subtitle | Mirror your Android phone on this computer and play its games with a keyboard and mouse. | Mirror your Android phone on your Mac and play with a keyboard and mouse. | 将 Android 手机投屏到 Mac，用键盘和鼠标玩手游。 |
| Page 0 · Feature 1 Title | Low-latency mirroring | Low-latency mirroring | 低延迟投屏 |
| Page 0 · Feature 1 Detail | Over USB or WiFi, with the phone's picture and sound. | Picture and sound over USB or Wi-Fi. | 通过 USB 或 Wi-Fi 传输画面和声音。 |
| Page 0 · Feature 2 Title | Keyboard and mouse for games | Keyboard & mouse controls | 键鼠操控 |
| Page 0 · Feature 2 Detail | A key mapping profile for each game, switched as you open it. | Per-game key profiles that switch automatically. | 按游戏匹配键位方案，打开游戏自动切换。 |
| Page 0 · Footer | Free and open source (GPL-3.0). Please follow local laws and each game's terms of use. | Free and open source (GPL-3.0). Follow local laws and each game's terms of use. | 开源免费（GPL-3.0），请遵守当地法规及游戏服务条款。 |
| Page 0 · Primary Button | Get started | Get started | 开始设置 |
| Page 1 · Title | Turn on USB debugging | Turn on USB debugging | 打开 USB 调试 |
| Page 1 · Subtitle | A one-time setup on the phone. Choose its brand for the exact steps. | One-time phone setup. Pick your brand for exact steps. | 仅需设置一次，选择手机品牌查看对应步骤。 |
| Page 1 · Search Hint | Can't find it? Search the phone's Settings for “Developer”. | Can't find it? Search Settings for “Developer”. | 找不到？在手机「设置」里搜索「开发者」。 |
| Page 2 · Title (`default`) | Connect your phone | Connect your phone | 连接手机 |
| Page 2 · Title (`unauthorized`) | Allow this computer on the phone | Allow this computer on your phone | 在手机上允许调试 |
| Page 2 · Title (`ready`) | %1 is ready | %1 is ready | %1 已就绪 |
| Page 2 · Title (`busy`) | Connecting to %1 | Connecting to %1 | 正在连接 %1 |
| Page 2 · Title (`streaming`) | %1 is on screen | %1 is mirroring | %1 正在投屏 |
| Page 2 · Title (`failed`) | Could not connect to %1 | Could not connect to %1 | 无法连接 %1 |
| Page 2 · Detail (`default`) | Plug the phone into this computer with a USB cable. It shows up here by itself. | Plug your phone in with a USB cable. It will appear automatically. | 用 USB 数据线连接手机和电脑，设备会自动出现。 |
| Page 2 · Detail (`offline`) | %1 is plugged in but not answering. Unplug the cable and plug it in again. | %1 is not responding. Unplug and replug the cable. | %1 未响应，请重新插拔数据线。 |
| Page 2 · Detail (`unauthorized`) | Unlock the phone. When it asks “Allow USB debugging?”, tick “Always allow from this computer” and tap Allow. | Unlock your phone, tick “Always allow from this computer”, and tap Allow. | 解锁手机，勾选「一律允许使用这台计算机进行调试」并点「允许」。 |
| Page 2 · Detail (`ready`) | Start mirroring to see and control the phone on this computer. | Click Start mirroring to view and control your phone. | 点击「开始投屏」即可在电脑上查看并操控手机。 |
| Page 2 · Detail (`busy`) | The first connection takes a few seconds. | First connection takes a few seconds. | 首次连接需要几秒钟。 |
| Page 2 · Detail (`streaming`) | Its window is open. You can close this guide. | Mirror window is open. You can close this guide. | 投屏窗口已打开，可以关闭此引导了。 |
| Page 2 · Waiting Beacon | Waiting for a phone… | Waiting for phone… | 等待连接手机… |
| Page 2 · Tip 1 | Use a cable that carries data; some cables only charge. | Use a data-capable USB cable (not charge-only). | 请使用支持数据传输的数据线（非仅充电线）。 |
| Page 2 · Tip 2 | If the phone asks what the USB connection is for, choose File transfer. | If prompted on the phone, choose File transfer. | 手机弹出 USB 用途提示时，选择「文件传输」。 |
| Page 2 · Tip 3 | Not set up on the phone yet? Go back for the steps. | Not set up yet? Go back to see the steps. | 还没开调试？点「上一步」查看步骤。 |
| Page 2 · Button (`See the steps`) | See the steps | See steps | 查看步骤 |
| Page 2 · Button (`Wi-Fi`) | Connect over WiFi instead | Connect over Wi-Fi | 改用 Wi-Fi 连接 |
| Page 2 · Primary (`ready`) | Start mirroring | Start mirroring | 开始投屏 |
| Page 2 · Primary (`failed`) | Try again | Try again | 重试 |
| Shared Buttons | Skip / Back / Next / Done | Skip / Back / Next / Done | 跳过 / 上一步 / 下一步 / 完成 |

---

## 2. `src/app/qml/TourOverlay.qml`

| Element | Old English | New English | New Chinese |
|---|---|---|---|
| Counter | Step %1 of %2 | %1 / %2 | 第 %1 / %2 步 |
| Shortcut label | Shortcut | Shortcut | 快捷键 |
| Buttons | Skip / Back / Next / Done | Skip / Back / Next / Done | 跳过 / 上一步 / 下一步 / 完成 |

---

## 3. `src/app/qml/DevicesPage.qml` (`devicesTour`)

| Step | Old English | New English | New Chinese |
|---|---|---|---|
| 1 · Title | Devices | Devices | 设备 |
| 1 · Text | Every phone connected to this computer gets a card here, and so do capture cards and the devices extensions add, such as touch boxes. A card shows up as soon as the device connects; there is nothing to refresh. | Connected phones, capture cards, and extension hardware appear here automatically as cards. | 已连接的手机、采集卡和扩展硬件会自动以卡片形式显示在这里。 |
| 2 · Title | Connect a phone | Connect a phone | 连接手机 |
| 2 · Text | Plug the phone in with a USB cable and turn on USB debugging in its developer options. The first time, the phone asks whether to allow USB debugging from this computer: tap Allow. | Plug in via USB with USB debugging on, then tap Allow on your phone's authorization prompt. | 用数据线连接并开启 USB 调试，首次连接时在手机上点「允许」。 |
| 3 · Title | Wireless pairing | Wireless pairing | 无线配对 |
| 3 · Text | No cable at hand? Turn on Wireless debugging on the phone and pair with the code or QR code it shows. After that the phone reconnects by itself on the same Wi-Fi. | Pair over Wi-Fi using a pairing code or QR code under Wireless debugging. | 没有数据线时，可通过「无线调试」的配对码或二维码配对。 |
| 4 · Title | How the picture travels | Connection mode | 传输方式 |
| 4 · Text | The adb tunnel goes through USB or wireless debugging and works everywhere. WiFi direct sends the picture straight over Wi-Fi with less delay. Auto picks for you. | Choose **adb tunnel** (works anywhere), **WiFi direct** (lower Wi-Fi latency), or **Auto**. | 可选 **adb 隧道**（通用）、**WiFi 直连**（局域网低延迟）或**自动**。 |
| 5 · Title | Start mirroring | Start mirroring | 开始投屏 |
| 5 · Text | Mirroring opens the phone's screen on this computer. Click and type there to work the phone, or pick a profile so that keys tap the screen for you. Try it now. | Open the phone's mirror window. Click and type directly, or use a key profile. Try it now. | 打开投屏窗口，可直接点击打字，或配合键位方案操控。 |
| 6 · Title | Settings for this phone | Device settings | 单机设置 |
| 6 · Text | Resolution, frame rate, bitrate and the other mirroring settings can be set for each phone. Whatever you leave alone follows the settings for all phones. | Override resolution, FPS, bitrate, or audio for this phone; other options follow global defaults. | 单独调整这台手机的分辨率、帧率、码率或声音，未改项沿用全局设置。 |
| 7 · Title | More for this phone | More options | 更多操作 |
| 7 · Text | Apps, files, keeping the screen on and ending a wireless connection are in this menu. | Open apps, browse files, keep the screen on, or disconnect wireless debugging here. | 在这里打开手机应用、管理文件、设置屏幕常亮或断开无线调试。 |
| 8 · Title | Grid | Grid view | 宫格视图 |
| 8 · Text | With several phones connected, Grid puts them side by side in one window so you can watch and work them together. | View and control multiple mirrored phones side by side in one window. | 同时连接多台手机时，用宫格视图在一个窗口里并排查看和操控。 |
| 9 · Title | That's the devices page | Devices tour complete | 设备页导览完成 |
| 9 · Text | The other pages are on the left: profiles hold your key mappings, tasks run things on a schedule, extensions add features. Each page has its own tour, and Help can replay them. | Use the left sidebar for Profiles, Tasks, and Extensions. Replay any tour anytime in Help. | 左侧可切换到方案、任务和扩展页；所有导览都能在「帮助」里重播。 |

---

## 4. `src/app/qml/ProfilesPage.qml` (`profilesTour`)

| Step | Old English | New English | New Chinese |
|---|---|---|---|
| 1 · Title | Profiles | Profiles | 方案 |
| 1 · Text | A profile is a set of keys: which key taps where on the screen and how it feels, optionally with logic and pictures to look for. A game can have several, and you can switch between them at any time. | Profiles store key mappings, logic, and vision targets for your games. Switch between them anytime. | 方案保存游戏的键位映射、自动化逻辑与识图资源，可随时切换。 |
| 2 · Title | New profile | New profile | 新建方案 |
| 2 · Text | Start from a template or a blank one, or let the AI assistant build one with you. Then edit its keys in the mirror window. | Create a profile from a template, blank canvas, or with the AI assistant, then edit keys on the mirror window. | 从模板、空白或 AI 助手开始创建方案，再去投屏窗口摆放按键。 |
| 3 · Title | Search | Search | 搜索 |
| 3 · Text | Narrow the list down by profile or game name. | Filter profiles by profile or game name. | 按方案名称或游戏名快速筛选。 |
| 4 · Title | Filters | Filters | 分类筛选 |
| 4 · Text | Show only the profiles for this game, the ones you made, the ones that came with extensions, or the ones with a problem to fix. | Filter by current game, your own profiles, extension profiles, or ones with errors. | 筛选当前游戏、自己创建的、来自扩展的或有问题的方案。 |
| 5 · Title | A profile card | Profile card | 方案卡片 |
| 5 · Text | The card tells which game the profile is for, which phone is using it and whether something needs fixing. | Shows the linked game, active devices, and any errors or warnings. | 显示关联的游戏、当前使用的设备以及错误提示。 |
| 6 · Title | Use it on a phone | Apply profile | 应用到设备 |
| 6 · Text | Use puts the profile on a phone that is mirroring. A profile linked to a game comes on by itself when the game opens. | Apply this profile to a mirrored phone. Linked profiles load automatically when the game opens. | 将方案应用到投屏中的手机；关联游戏后打开该游戏会自动切换。 |
| 7 · Title | More for a profile | More actions | 更多操作 |
| 7 · Text | Linking a game, vision resources, editing logic, exporting, restoring the original and deleting are in this menu. | Link a game, edit vision resources or logic, export, restore originals, or delete here. | 在这里关联游戏、编辑视觉资源与逻辑、导出、恢复原版或删除。 |
| 8 · Title | Import | Import | 导入 |
| 8 · Text | Add a profile file someone shared with you; dropping the file on the window works too. | Import a shared `.json` or `.gsprofile` file, or drop it onto the window. | 导入别人分享的 `.json` 或 `.gsprofile` 文件，也支持直接拖入窗口。 |
| 9 · Title | That's profiles | Profiles tour complete | 方案页导览完成 |
| 9 · Text | Share a profile you made by exporting it, or pack it as an extension. Help can replay this tour. | Export profiles to share or package them as extensions. Replay this tour in Help. | 导出方案即可分享或打包为扩展，可在「帮助」中重播此导览。 |

---

## 5. `src/app/qml/TasksPage.qml` (`tasksTour`)

| Step | Old English | New English | New Chinese |
|---|---|---|---|
| 1 · Title | Tasks | Tasks | 任务 |
| 1 · Text | A task is logic that runs by itself: check in every day, farm a level while you are away, remind you of something. It can run on the computer alone or work a phone that is mirroring. | Tasks run automation scripts independently—for daily check-ins, farming, or reminders—without interrupting your key mappings. | 任务独立于键位方案运行，适合每日签到或挂机，不抢占手动操作。 |
| 2 · Title | New task | New task | 新建任务 |
| 2 · Text | Start from a blank task and write your own logic, or copy an example and change it. | Create a blank task or start from an example. | 创建空白任务或从示例复制修改。 |
| 3 · Title | A task card | Task card | 任务卡片 |
| 3 · Text | The card shows the task's name, when it runs and whether it is running now. | Shows the task name, schedule, and live run status. | 显示任务名称、定时计划和当前运行状态。 |
| 4 · Title | Run and stop | Start & stop | 启动与停止 |
| 4 · Text | Run a task by hand or stop it at any time. | Start or stop a task manually anytime. | 随时手动启动或停止任务。 |
| 5 · Title | Task settings | Task settings | 任务设置 |
| 5 · Text | Choose when the task runs, such as every day or on some days of the week, which phone it works, and open its logic to edit it. | Set the schedule, target phones, parameters, and edit the task's logic here. | 设置定时时间、目标手机和参数，或打开逻辑编辑器。 |
| 6 · Title | Recent runs | Recent runs | 最近运行 |
| 6 · Text | Every run is kept here: when it started, on which phone, how long it took and how it ended. | History of past runs with start time, phone, duration, and outcome. | 记录每次运行的开始时间、设备、耗时和结果。 |
| 7 · Title | That's tasks | Tasks tour complete | 任务页导览完成 |
| 7 · Text | With the setting for it on, closing the window leaves anmacgamer in the menu bar so that tasks with a schedule still run on time. Help can replay this tour. | Enable menu bar mode in Settings so scheduled tasks keep running when the main window is closed. | 在设置中开启「关闭到菜单栏」后，关掉主窗口也能按时执行定时任务。 |

---

## 6. `src/app/qml/ExtensionsPage.qml` (`extensionsTour`)

| Step | Old English | New English | New Chinese |
|---|---|---|---|
| 1 · Title | Extensions | Extensions | 扩展 |
| 1 · Text | Extensions add to anmacgamer: new components, picture filters, themes, hardware such as a touch box, side panels, and new blocks for profiles and logic. | Extensions add touch boxes, components, themes, panels, AI providers, and logic blocks. | 扩展可添加触控盒、组件、主题、侧边面板、AI 服务与逻辑节点。 |
| 2 · Title | An extension card | Extension card | 扩展卡片 |
| 2 · Text | The card tells what the extension adds and what it may do. | Shows what the extension adds and the permissions it uses. | 展示扩展提供的功能以及它申请的权限。 |
| 3 · Title | Turn it on | Enable extension | 启用扩展 |
| 3 · Text | An extension only loads once its switch is on. Turning it on allows the permissions it asks for. | Extensions load only when switched on, which grants their requested permissions. | 打开开关才会加载扩展并授予其所需权限。 |
| 4 · Title | Settings | Settings | 设置 |
| 4 · Text | Extensions that have settings are set up here. | Configure options provided by this extension. | 配置该扩展提供的选项。 |
| 5 · Title | Details | Details | 详情 |
| 5 · Text | See everything it adds, the permissions it asked for and any problem it ran into. | View added features, permissions, crash history, and updates. | 查看新增功能、权限明细、健康记录与更新信息。 |
| 6 · Title | Install from file | Install from file | 从文件安装 |
| 6 · Text | Add an extension from a .gsext package. Before it installs, it shows what the extension may do. | Install a `.gsext` or `.gsbundle` package after reviewing its permissions. | 选择 `.gsext` 或 `.gsbundle` 文件，核对权限后安装。 |
| 7 · Title | Find a problem extension | Find a problem extension | 查找问题扩展 |
| 7 · Text | When something goes wrong, this turns half of the extensions off at a time to find the one that causes it. | Binary-search your enabled extensions to isolate the one causing an issue. | 通过二分法逐批停用扩展，快速定位引发故障的扩展。 |
| 8 · Title | Export bundle | Export bundle | 导出合集 |
| 8 · Text | Pack the extensions you use into one bundle file to set up another computer or share them. | Pack all enabled extensions and settings into one `.gsbundle` file. | 把当前启用的扩展和设置打包为一个 `.gsbundle` 文件。 |
| 9 · Title | That's extensions | Extensions tour complete | 扩展页导览完成 |
| 9 · Text | Extensions only do what you allowed. Help can replay this tour. | Extensions only run with permissions you approved. Replay this tour in Help. | 扩展仅在你授权的范围内运行，可在「帮助」中重播此导览。 |

---

## 7. `src/app/qml/SettingsPage.qml` (`settingsTour`)

| Step | Old English | New English | New Chinese |
|---|---|---|---|
| 1 · Title | Settings | Settings | 设置 |
| 1 · Text | Every setting is here, and a change takes effect right away; there is nothing to save. | Changes take effect immediately—no save button needed. | 所有设置修改后立即生效，无需手动保存。 |
| 2 · Title | Search settings | Search settings | 搜索设置 |
| 2 · Text | Not sure where a setting is? Search for it, such as frame rate or key hints. | Search by keyword, such as “frame rate” or “key hints”. | 输入关键词（如「帧率」或「键位提示」）快速定位设置项。 |
| 3 · Title | Groups | Categories | 设置分组 |
| 3 · Text | The settings are grouped: mirroring and quality, audio, input and keys, shortcuts, general, appearance, extensions and AI. A number next to a group counts the settings a phone has changed for itself. | Browse by category. A badge shows how many settings are overridden for the selected phone. | 按功能分类浏览；分组旁的数字表示当前手机单独覆盖的设置数量。 |
| 4 · Title | The settings | Options | 设置项 |
| 4 · Text | Each row says what it is for on the left and has its control on the right. | Each row explains the option on the left with its control on the right. | 左侧说明用途，右侧直接调节开关或数值。 |
| 5 · Title | All phones or this phone | Global vs. per-phone | 全局或单机 |
| 5 · Text | With a phone connected, choose this phone to change a setting for it alone; the rest follow the default for all phones. Reset takes a changed setting back to that default. | Switch to a connected phone to override settings just for it, or reset back to global defaults. | 连接手机后可切换为单机设置，其余未改项仍沿用全局默认。 |
| 6 · Title | That's settings | Settings tour complete | 设置页导览完成 |
| 6 · Text | The AI assistant's service and key are set up under AI. Help can replay this tour. | Configure your AI model and MCP access under **AI**. Replay this tour in Help. | 在 **AI** 分组配置模型服务和 MCP，可在「帮助」中重播此导览。 |

---

## 8. `src/app/qml/MirrorOverlay.qml` (`mirrorTour`)

| Step | Old English | New English | New Chinese |
|---|---|---|---|
| 1 · Title | The mirror window | Mirror window | 投屏窗口 |
| 1 · Text | This is the phone's screen. Clicking, dragging and scrolling here taps, swipes and scrolls on the phone, and what you type goes into the phone. The toolbar shows when the mouse moves to the top of the window. | Click, drag, scroll, and type here to control your phone. Move the pointer to the top to reveal the toolbar. | 点击、拖拽、滚动和打字都会直接传给手机；鼠标移到顶部可显示工具栏。 |
| 2 · Title | Key mapping | Key mapping | 键位映射 |
| 2 · Text | With key mapping on, the keyboard and mouse tap the screen the way the profile says, which is how you play games. Turn it off to type into the phone. Try it: click it twice. | Toggle game key mapping on or off (turn off to type normally). Try clicking it twice. | 开启后用键鼠按方案玩游戏，关闭即可正常点击打字。试着点两次看看。 |
| 3 · Title | Key mapping options | Key mapping options | 键位映射选项 |
| 3 · Text | Switch profiles, show or hide the key hints, and open the editor where you place the keys on the picture. | Switch profiles, adjust key hints, or open the key mapping editor. | 切换方案、调整键位提示透明度或打开键位编辑器。 |
| 4 · Title | Back, Home and Recent apps | Navigation | 系统导航 |
| 4 · Text | The same as the three navigation keys at the bottom of the phone. | Trigger Back, Home, and Recent apps. | 对应手机底部的返回、主页和最近任务键。 |
| 5 · Title | Rotate | Orientation | 旋转方向 |
| 5 · Text | Turn the picture, or let the phone rotate by itself; the window changes shape with it. | Follow device rotation or lock Portrait / Landscape. | 跟随手机旋转，或锁定为竖屏/横屏。 |
| 6 · Title | Turn the phone's screen off | Screen off | 熄屏投屏 |
| 6 · Text | The phone's own screen goes dark while you keep mirroring and working it here. It saves power and keeps the screen private. | Turn off the phone's physical screen while mirroring continues. | 关闭手机实体屏幕以省电和保护隐私，电脑端继续正常投屏。 |
| 7 · Title | Volume | Volume | 音量 |
| 7 · Text | The phone's sound plays on this computer. Change how loud it is here or mute it; the phone's own volume stays as it is. | Control phone audio on your Mac; the phone's physical volume keys do not change it while mirroring. | 在 Mac 上调节或静音投屏声音；投屏期间手机自身音量键不会改变音量。 |
| 8 · Title | Tasks | Tasks | 任务 |
| 8 · Text | Run a task on this phone, such as a script that plays for you, and see or stop the ones that are running. | Start or stop automation tasks running on this phone. | 在这台手机上启停自动化任务并查看进度。 |
| 9 · Title | Record | Record | 录制 |
| 9 · Text | Record the picture and sound to a video. A recording also lets you test logic later without the phone. | Record video, audio, and inputs (also useful for replaying scripts offline). | 录制画面、声音与操作，也可用于后续离线回放测试脚本。 |
| 10 · Title | Extension panels | Extension panels | 扩展面板 |
| 10 · Text | Extensions that come with a panel open it beside the picture from here. | Open side panels added by enabled extensions. | 打开已启用扩展提供的侧边面板。 |
| 11 · Title | Diagnostics | Diagnostics | 诊断信息 |
| 11 · Text | Shows the frame rate, the delay and how the picture is decoded. When things stutter, look here first. | Show live FPS, latency, and decoder stats—check here first if video stutters. | 显示实时帧率、延迟和解码状态，卡顿时先看这里。 |
| 12 · Title | Full screen | Full screen | 全屏 |
| 12 · Text | The picture fills the whole screen; do it again to come back. | Fill the screen with the phone display; press again to exit. | 让手机画面占满整个屏幕，再按一次退出。 |
| 13 · Title | More | More | 更多 |
| 13 · Text | The notification shade, quick settings, recordings, editing logic, the vision workbench, files, the small window, syncing phones and this phone's settings are in here. When the window is narrow, the buttons that do not fit are in here too. | Access recordings, logic editor, vision workbench, mini window, toolbar customization, and device settings. | 查看录制内容、编辑逻辑、视觉工作台、置顶小窗、自定义工具栏按钮和设备设置。 |
| 14 · Title | Pin the toolbar | Pin toolbar | 固定工具栏 |
| 14 · Text | Pinned, the toolbar stays above the picture instead of hiding until the mouse comes up. | Keep the toolbar visible above the screen instead of auto-hiding. | 固定后工具栏会一直停留在画面上方，不再自动隐藏。 |
| 15 · Title | Ready to play | Ready to play | 准备就绪 |
| 15 · Text | To have keys tap the buttons of a game, open Edit key mapping under Key mapping options and drag keys onto the picture. Help can replay this tour. | Open **Key mapping options > Edit key mapping** to place keys on game buttons. Replay this tour in Help. | 打开「键位映射选项 > 编辑键位映射」即可在画面上摆放按键，可在「帮助」中重播导览。 |

---

## 9. `src/app/qml/KeymapEditor.qml` (`keymapTour`)

| Step | Old English | New English | New Chinese |
|---|---|---|---|
| 1 · Title | Editing the key mapping | Key mapping editor | 键位编辑器 |
| 1 · Text | Here you set up the keys for a game: drag components onto the phone picture and give each one a key. Every change can be undone, and Save writes them into the profile. | Drag components onto game buttons and bind keys. Every edit can be undone; click **Save** to keep changes. | 把组件拖到游戏按钮上并绑定按键，支持随时撤销，点「保存」写入方案。 |
| 2 · Title | The component library | Component library | 组件库 |
| 2 · Text | Tap, hold, turbo, swipe, the WASD wheel, the camera, skills… each component presses the screen its own way; rest the mouse on one to see what it does. Try it: drag one onto the picture. | Hover over any component to see how it works, then drag one onto the screen. | 悬停组件可查看说明，试着拖一个到右侧画面上。 |
| 3 · Title | Place it and give it a key | Place & bind | 放置并绑定按键 |
| 3 · Text | Drag the component onto the game's button, then click it and press the key you want to use for it. | Drop it over a button, then press a keyboard key or mouse button to bind it. | 放到游戏按钮位置后，直接按下键盘按键或鼠标键完成绑定。 |
| 4 · Title | Properties | Properties | 属性面板 |
| 4 · Text | The selected component's settings: its key, its name and how it feels, such as how fast a turbo taps or how far a swipe goes. Use it in logic is here too. | Adjust key, sensitivity, anchor corner, code name, or jump to **Use it in logic**. | 调整所选组件的按键、灵敏度、锚点、代码名称，或直接「在逻辑中使用」。 |
| 5 · Title | Placed | Placed keys | 已放置列表 |
| 5 · Text | Placed lists every component on the picture with its key, and marks keys that clash. | Lists all placed components, their bound keys, and any key conflicts. | 查看画面上所有已放置的组件、对应按键及冲突标记。 |
| 6 · Title | Picture and logic | Picture & Logic | 画面与逻辑 |
| 6 · Text | Logic connects keys to a blueprint: a press can run a series of actions, check conditions or look for pictures on the screen. The components' own events are there too. | Switch to **Logic** to build combos, conditions, and image-matching blueprints or TypeScript. | 切换到「逻辑」可编写连招、条件判断与识图找色脚本或蓝图。 |
| 7 · Title | Try the keys | Try live | 实时试玩 |
| 7 · Text | Try these keys on the phone without leaving the editor; click again to go back to editing. | Test your keys on the live game without leaving the editor; click again to resume editing. | 无需退出编辑器即可直接在游戏上测试按键，再点一次回到编辑。 |
| 8 · Title | Undo and redo | Undo & Redo | 撤销与重做 |
| 8 · Text | Every change can be taken back, and brought back again. | Undo or redo any change. | 撤销或重做任意一步编辑。 |
| 9 · Title | Freeze the picture | Freeze picture | 定格画面 |
| 9 · Text | When the game keeps moving, freeze the picture to line the components up with its buttons. | Pause the live screen so you can align keys over moving UI. | 画面不断变化时可先定格，方便精准对齐按钮。 |
| 10 · Title | More | More | 更多 |
| 10 · Text | Save as, restoring the original profile, handing a gamepad to the game and discarding the changes are in here. | Save as, restore original, toggle gamepad passthrough, or discard changes here. | 提供另存为、恢复原版、手柄直通游戏及放弃修改等选项。 |
| 11 · Title | Save | Save & Done | 保存与完成 |
| 11 · Text | Save writes the changes into the profile; Done saves and goes back to mirroring. | **Save** writes changes now; **Done** saves and returns to the mirror window. | **保存**立即写入方案，**完成**会保存并退出编辑器。 |
| 12 · Title | Ready to play | Ready to play | 开始游戏 |
| 12 · Text | With the keys in place, click Done and play. For combos and smarter keys, switch to Logic. Help can replay this tour. | Click **Done** to play, or switch to **Logic** for automation. Replay this tour in Help. | 点击「完成」即可开玩，需要连招或自动化可切到「逻辑」，可在「帮助」中重播导览。 |

---

## 10. `src/app/qml/LogicView.qml` (`logicTour`)

| Step | Old English | New English | New Chinese |
|---|---|---|---|
| 1 · Title | The logic view | Logic view | 逻辑视图 |
| 1 · Text | Logic lets keys do more: a combo on one press, a tap when a picture shows up, a different move depending on what is happening. The blueprint draws the logic as blocks; it is the code, and you can edit either one. | Build combos, conditional actions, and vision scripts visually as a blueprint or directly in TypeScript. | 用可视化蓝图或 TypeScript 编写连招、条件分支与识图脚本，两者实时互通。 |
| 2 · Title | My blueprint | My blueprint | 我的蓝图 |
| 2 · Text | Components, variables, functions, the function library and the pages are here. Click a component to open its own file, where its key events go. | Browse components, variables, functions, and libraries. Click a component to edit its key events. | 在这里查看组件、变量、函数和函数库；点击组件可打开其专属事件文件。 |
| 3 · Title | Place a block | Place a block | 放置节点 |
| 3 · Text | Right-click an empty spot, or drag an item from the left, to place a block. Rest the mouse on a block in the list to see what it does. Try it: place one. | Right-click the canvas or drag from the left sidebar to add a block. Try placing one now. | 在空白处右键或从左侧拖入项目来添加节点，试着放一个看看。 |
| 4 · Title | Connect the blocks | Connect blocks | 连接节点 |
| 4 · Text | Drag from the white arrow on the right of a block to the left of the next one; the blocks run from left to right. The colored dots carry values and connect the same way. | Drag from a white execution arrow to the next block's input; colored pins pass data values. | 从白色执行箭头拖向下一个节点即可串联流程，彩色圆点用于传递数据。 |
| 5 · Title | Pages | Pages | 页面切换 |
| 5 · Text | The main page, the component files and every function have a page of their own; switch between them here. | Switch between the main page, component files, and function tabs here. | 在主页面、各组件文件和函数标签页之间切换。 |
| 6 · Title | Blueprint and code | Blueprint & Code | 蓝图与代码 |
| 6 · Text | Switch to the code to see the TypeScript behind the blueprint. Change the code and the blueprint follows. | Switch between visual **Blueprint** and **Code**—changes in either view stay in sync. | 切换可视化**蓝图**与 **Code**（TypeScript 代码），任一端修改都会自动同步。 |
| 7 · Title | Run and debug | Run & debug | 运行与调试 |
| 7 · Text | Run starts the logic on the phone, Run on a recording tests it against a recorded video, and the live list shows what each component is doing. Set breakpoints on blocks to watch the values step by step. | Run live on the phone, test against a recording, inspect live states, or step through breakpoints. | 在手机上运行、用录像回放测试、查看组件实时状态，或打断点单步调试。 |
| 8 · Title | Console, problems and references | Console & problems | 控制台与问题 |
| 8 · Text | Logs go to the console. Mistakes are listed under Problems; click one to go to it. | View `log` output in **Console** and click any error under **Problems** to jump to it. | 在**控制台**查看日志输出，点击**问题**列表中的报错可直接跳转。 |
| 9 · Title | Start building | Start building | 开始编写 |
| 9 · Text | Not sure what a block does? Rest the mouse on it, or ask the AI assistant at the bottom right. Help can replay this tour. | Hover over blocks for tips or ask the AI assistant. Replay this tour in Help. | 悬停节点可看说明，也可随时问右下角的 AI 助手；可在「帮助」中重播导览。 |

---

## 11. `src/app/qml/AiCorner.qml` (`assistantTour`)

| Step | Old English | New English | New Chinese |
|---|---|---|---|
| 1 · Title | The AI assistant | AI assistant | AI 助手 |
| 1 · Text | It knows which window you are in and which profile you are looking at. Ask it to set up keys, write blueprints and scripts, explain blocks or find out why a phone does not connect. Whatever it changes goes into a draft first and only counts once you agree. | Understands your current window and profile. Ask it to set up keys, write scripts, or troubleshoot—edits wait in a draft until you apply them. | 能感知当前窗口与方案，可帮你配键位、写脚本或排查连接问题；所有修改先存为草稿，确认后才生效。 |
| 2 · Title | New conversation | New chat | 新建对话 |
| 2 · Text | Start a new conversation for a new topic; the earlier ones are kept. | Start a fresh conversation; earlier chats stay saved. | 开启新话题，历史对话会自动保留。 |
| 3 · Title | Recent conversations | Recent chats | 最近对话 |
| 3 · Text | Every conversation is kept on this computer. The newest %n are listed here; Show all opens the rest in a separate window. Pick one up again or delete them. | Reopen the latest %n saved chats here, or click **Show all** to manage everything in a window. | 列出最近 %n 条本地对话，点「显示全部」可在独立窗口查看或清理。 |
| 4 · Title | A separate window | Pop-out window | 独立窗口 |
| 4 · Text | Chat in a window of its own, with the recent conversations on the left; it remembers its size and place. | Open the assistant in its own resizable window with the chat list on the left. | 在独立窗口中聊天，左侧保留历史列表并记住窗口大小与位置。 |
| 5 · Title | Collapse | Collapse | 收起 |
| 5 · Text | The card folds away and the bubble stays in the corner. The shortcut brings it back in any window. | Minimize to the corner bubble; press the shortcut anytime to reopen. | 收起为角落悬浮球，随时按快捷键重新呼出。 |
| 6 · Title | Ask | Ask | 提问 |
| 6 · Text | Type a question below and press Enter. While it answers you can follow its thinking and the tools it uses. If no service is set up yet, set one up under AI in Settings first. | Type a question and press Enter. Configure your AI service under **Settings > AI** if needed. | 在下方输入问题并按 Enter；若尚未配置模型，请先前往**设置 > AI**。 |
