# GameScrcpy for macOS App 图标设计方案（G8）

- **日期**：2026-11-24（2026-11-25 补充定案：方向 B 双端方案）
- **结论**：初版提出 4 个原生适配 macOS 26 Liquid Glass 的图标方向并推荐方向 A；**2026-11-25 用户已拍板采用方向 B「双屏跃迁与极速帧流」作为双端统一构图**——Mac 端采用 `#0A84FF` 系统蓝配色，手机端（Android / Companion）采用 **配色方案 1「极光翠绿（Emerald Signal）」**（`#10B981` → `#34D399`）。本文第 5 节已补充完整的 **方向 B macOS Icon Composer 4 层 SVG**（[`icon/layers-b-macos/`](icon/layers-b-macos/)）以及 **Android Adaptive Icon 分层规范、色值与 SVG 素材**（[`icon/layers-b-android/`](icon/layers-b-android/)），原方向 A 方案与素材保留在第 4 节供归档参考。

---

## 1. 设计依据与约束核对

依据 `TEAM.md` 第 4 节 G8 及 Apple 官方 [Human Interface Guidelines — App icons](https://developer.apple.com/design/human-interface-guidelines/app-icons) 与 [Creating your app icon using Icon Composer](https://developer.apple.com/documentation/xcode/creating-your-app-icon-using-icon-composer)：

- **产品定位与用户画像**：将安卓手机（以游戏为主）低延迟、原画质、高刷新率投屏至 Mac，通过 Mac 键盘、鼠标、触控板与手柄操控，并提供键位映射、自动化脚本与录屏。用户既包含重度手游玩家，也包含习惯在 Mac 上操作手机的效率用户。
- **界面语言**：macOS 26 Liquid Glass 风格，强调色为系统蓝 `#0A84FF`。
- **技术与合规限制**：
  1. **Icon Composer 分层结构**：1 个背景填充 + 最多 4 组前景图层，画布为 `1024×1024` 全出血正方形透明 PNG 或 SVG；**不在素材中烘焙圆角遮罩、外投影或固定玻璃高光**，由系统统一生成圆角、安全边距、折射与六种外观（Default、Dark、Clear Light、Clear Dark、Tinted Light、Tinted Dark）。
  2. **无文字、无侵权元素**：不含任何字母/文字，不使用安卓机器人、品牌标志或游戏角色。
  3. **极小尺寸可读性**：缩放至 `16×16 px` 时仍可清晰识别轮廓与中心特征。
- **现有临时标志评估（`docs/images/logo.png`）**：当前为深黑底色 + 白色横屏手机线框 + 左侧听筒圆点 + 居中橙色准星。优点是“横屏游戏操控”语义明确；不足在于扁平线框缺乏 macOS 26 玻璃景深、橙色与软件 `#0A84FF` 系统蓝割裂、听筒小圆点与细线在 16 px 下易消失。

> **关于概念图存放说明**：4 个方向的高精度 Liquid Glass 渲染概念图已在本次 Gemini 对话中直接输出供直观审阅；同时，[`icon/preview-a-precision-cast.svg`](icon/preview-a-precision-cast.svg) 与 [`icon/layers/`](icon/layers/) 中已保存推荐方案（方向 A）的合层结构矢量预览及 4 组 `1024×1024` Icon Composer 分层 SVG 文件，供 Claude 直接导入使用。

---

## 2. 四个候选方向

### 方向 A：棱镜投屏准星（Precision Cast）——【推荐】

- **风格定位**：精密光学仪器 × macOS 26 原生 Pro 工具质感（沉稳、克制、高精度、低延迟）。
- **核心寓意**：将“双屏无缝投射”与“键鼠/手柄精准操控”融于一体——后层微偏的蓝色玻璃面板代表手机源屏，前层居中的磨砂液态玻璃横屏代表 Mac 上的原画质高刷视窗，正中一枚加粗几何光学准星与 `#0A84FF` 发光焦点圆点象征零延迟瞄准、键位映射与精准触发。
- **构图**：
  - **背景**：深空曜石蓝黑纵向渐变，中央带微弱 `#0A84FF` 环境光晕，为前方透亮液态玻璃提供高对比舞台。
  - **前景**：后层左上偏移的半透明蓝玻璃横屏（源屏）+ 前层居中大比例冷白磨砂玻璃横屏（投屏视窗）+ 视窗正中四段断开圆环配四向加粗刻度线的纯白准星 + 准星中心高亮系统蓝实心圆点。
- **具体色值**：
  - 背景渐变：`#0D1526`（顶）→ `#162640`（底）
  - 后层源屏：`#2B7BFF`（88%）→ `#0A84FF`（68%）
  - 主体投屏玻璃：`#F5F9FF`（42%）→ `#DCEBFF`（24%），内描边 `#FFFFFF`（45%）
  - 光学准星：`#FFFFFF`（96%）
  - 中心焦点圆点：`#6CD4FF`（中心高光）→ `#0A84FF`（主色）→ `#005FD6`（边缘）
- **为什么适合**：
  - 语义最精准：一眼同时传达“横屏投屏”与“精准操控/游戏映射”两大核心卖点，且不局限于单一输入设备（键盘、鼠标、触控板、手柄均适用）。
  - 视觉一致性最高：深蓝玻璃舞台 + `#0A84FF` 核心点与软件 Liquid Glass 界面高度统一。
  - 16 px 表现稳健：去掉了原版细小的听筒圆点，将准星与中心蓝点加粗放大，16 px 下呈现“深蓝底 + 亮色横屏 + 蓝白中心焦点”的清晰剪影。

---

### 方向 B：双屏跃迁与极速帧流（Dual-Frame Stream）

- **风格定位**：苹果系统原生效率工具风（接近 AirPlay、Sidecar、Screen Mirroring 的轻盈家族感）。
- **核心寓意**：后方紧凑的深蓝竖屏代表掌上手机，前方更大、更透亮的系统蓝液态玻璃横屏代表 Mac 投屏大视窗，玻璃面上悬浮一枚简洁白色向右流光折角（`>`），隐喻“从手机跃迁至 Mac 大屏的高刷流畅画面与录屏/运行”。
- **构图**：
  - **背景**：清透冰白至浅天蓝渐变，在深色壁纸与深色 Dock 上格外醒目。
  - **前景**：左后方深海军蓝圆角竖屏 + 右前方大比例半透明 `#0A84FF` 液态玻璃横屏（两者交叠产生明显玻璃折射纵深）+ 横屏正中纯白粗线条向右折角符号。
- **具体色值**：
  - 背景渐变：`#F0F6FF`（顶）→ `#CFE4FF`（底）
  - 后层手机竖屏：`#1D3557`（顶）→ `#0F2038`（底）
  - 前层投屏大玻璃：`#0A84FF` → `#38BDF8`（液态玻璃半透明）
  - 流光折角符号：`#FFFFFF`（100%）
- **为什么适合**：
  - 对“把手机投到 Mac 日常办公/操作 + 偶尔玩游戏”的泛用户最友好，毫无传统游戏工具的侵略感。
  - 浅色底与系统原生应用并排摆放极其和谐；16 px 下“浅底 + 深蓝/亮蓝双矩形叠层”轮廓分明。
  - **取舍点**：偏向通用投屏/视频工具，“游戏操控与键位映射”特征较弱。

---

### 方向 C：玻璃操控视窗（Tactile Controller Glass）

- **风格定位**：精致微拟物硬件风 × 现代 Liquid Glass（延续 macOS 经典硬件图标美学，克制圆润，拒绝廉价电竞尖角）。
- **核心寓意**：将横屏手机与游戏手柄合二为一，塑造成一台极简半透明银白液态玻璃掌机/操控器，中央嵌入发光的系统蓝宽屏视窗，左侧为极简白色十字方向键，右侧为两颗圆润按键（白色 + 电光青），直接点明“用 Mac 玩手游与手柄/按键映射”。
- **构图**：
  - **背景**：深钛灰至曜石黑渐变，突出冷银白玻璃机身的精密硬件质感。
  - **前景**：居中饱满的圆润流线型液态玻璃操控器主体 + 中央内嵌系统蓝宽屏玻璃视窗 + 左侧加粗白色圆角十字块 + 右侧双圆键。
- **具体色值**：
  - 背景渐变：`#23262F`（顶）→ `#101218`（底）
  - 机身主玻璃体：`#E5ECFF`（半透明冷银白磨砂玻璃）
  - 中央屏幕视窗：`#0A84FF`（中心亮部）→ `#0055D4`（深钴蓝边缘）
  - 左十字键 / 右主按键：`#FFFFFF`（96%）
  - 右副按键点缀：`#32D7FF`（电光青）
- **为什么适合**：
  - 游戏玩家一眼识别，直击“在 Mac 上玩手游”的心智。
  - 单一饱满中心主体，在 16 px 下仍可辨认出“手柄轮廓 + 中心蓝屏”。
  - **取舍点**：过于偏向纯游戏场景，对仅想用 Mac 办公操作手机的用户略显娱乐化；十字键与圆键细节在 16 px 下会退化为色点。

---

### 方向 D：零延迟电光菱镜（Zero-Latency Pulse）

- **风格定位**：高度抽象几何徽章 × 现代 Mac 极客工具美学（类似 Raycast、Linear 的现代标志感）。
- **核心寓意**：彻底跳脱手机或手柄的具象外形，用两片反向交错的蓝色液态玻璃菱形光圈构成高速快门/信号脉冲形态，包裹中央纯白圆角菱形焦点，象征“极速低延迟、高刷新率、自动化脚本与精准指令触发”。
- **构图**：
  - **背景**：极夜黑蓝至深靛蓝渐变。
  - **前景**：底层深海蓝圆角菱形光片 + 中层交错叠压的亮系统蓝至极光青透明玻璃折翼 + 顶层正中发光纯白圆角菱形核心。
- **具体色值**：
  - 背景渐变：`#090E1A`（顶）→ `#13203D`（底）
  - 底层光翼：`#0051CC` → `#0A84FF`
  - 中层折射翼：`#0A84FF` → `#38D9FF`
  - 顶层焦点核心：`#FFFFFF`（100%）
- **为什么适合**：
  - 品牌感与前卫感最强，不受硬件形态演进影响，在 16 px、菜单栏图标和 Tinted 模式下表现极其稳固。
  - **取舍点**：抽象度较高，首次见到的用户无法仅凭图标立刻联想到“手机投屏”。

---

## 3. 四方向横向对比

| 维度 | 方向 A：棱镜投屏准星（推荐） | 方向 B：双屏跃迁 | 方向 C：玻璃操控视窗 | 方向 D：电光菱镜 |
|---|---|---|---|---|
| **核心识别符号** | 双层横屏 + 准星 + 蓝核 | 竖屏手机 + 蓝玻璃横屏 + 折角 | 玻璃掌机 + 蓝屏 + 十字/双键 | 交错玻璃菱镜 + 白核 |
| **视觉气质** | 精密 Pro 工具 / 冷静锐利 | 苹果原生系统效率工具 | 精致现代游戏硬件 | 现代极客 / 抽象品牌徽章 |
| **游戏 vs 日常平衡** | **最佳**（游戏精准，日常也专业） | 偏日常投屏，游戏感较弱 | 偏重手游玩家，办公感弱 | 偏极客脚本，投屏指向弱 |
| **Liquid Glass 层次** | 4 层纵深清晰，折射自然 | 3 层叠压折射极强 | 3 层硬件浮雕质感好 | 3 层光影交错通透感强 |
| **16 px 辨识度** | 高（深底 + 亮横屏 + 中心焦点） | 高（浅底 + 双色块叠层） | 中高（外轮廓清，按键变色点） | 极高（对称高对比菱形） |
| **与旧 `logo.png` 延续度** | 高（保留“横屏+准星”基因并彻底升级） | 中（保留投屏概念） | 低（转向手柄形态） | 低（完全抽象重构） |

---

## 4. 最终推荐与 Apple Icon Composer 图层拆解

### 推荐结论：采用方向 A「棱镜投屏准星（Precision Cast）」

方向 A 在**用途准确度（投屏 + 操控）**、**macOS 26 Liquid Glass 原生质感**、**`#0A84FF` 品牌色融合**以及**与现有 `docs/images/logo.png` 的平滑演进**四个维度上综合表现最均衡。

### Icon Composer 构建规范（给 Claude 直接实操）

依据 [Creating your app icon using Icon Composer](https://developer.apple.com/documentation/xcode/creating-your-app-icon-using-icon-composer)：
- **画布尺寸**：`1024 × 1024 px` 全出血正方形（不要裁切圆角，不要外加阴影）。
- **安全区**：全部前景元素位于 `[164, 216]` 至 `[840, 748]` 范围内（四周均留出 ≥ 164 px 安全余量，确保 macOS 26 圆角遮罩与视差位移绝不裁边）。
- **现成矢量素材路径**：`docs/handoff/gemini/icon/layers/` 下已按从底到顶顺序编号，直接拖入 Icon Composer 左侧 Sidebar 即可自动生成 4 个独立 Group。

#### 背景填充（Background Inspector）

| 外观模式 | 填充类型 | 色值设置 | 说明 |
|---|---|---|---|
| **Default / Dark** | Linear Gradient（上 → 下） | 顶部 `#0D1526` → 底部 `#162640` | 深空藏蓝衬托前景玻璃高光与 `#0A84FF` 焦点 |
| **Clear Light / Clear Dark** | System Automatic | 由系统自适应 | 依赖前景 4 层清晰不透明度差保持识别度 |
| **Tinted Light / Tinted Dark** | System Automatic | 由系统着色 | 层 3 准星与层 4 核心的高亮差可保持轮廓清晰 |

#### 前景 4 组图层清单（由底至顶 Z 轴顺序）

| 组序 | 素材文件 | 内容说明 | 坐标与尺寸（1024 画布） | 基础色值 | Icon Composer 材质参数建议 |
|---|---|---|---|---|---|
| **Group 1** | [`01-source-screen.svg`](icon/layers/01-source-screen.svg) | **后层源屏**：向左上微偏的蓝色玻璃圆角矩形，隐喻手机端源画面与投屏景深 | `x=164, y=216`<br>`w=580, h=420`<br>`rx=52` | 线性渐变 `#2B7BFF`（88%）→ `#0A84FF`（68%） | Mode: `Combined`<br>Liquid Glass: `On`<br>Translucency: `65%`<br>Blur: `20%`<br>Specular: `Automatic`<br>Shadow: `Subtle` |
| **Group 2** | [`02-main-screen.svg`](icon/layers/02-main-screen.svg) | **主体投屏玻璃视窗**：居中偏右下微移的冷冰白磨砂液态玻璃横屏，代表 Mac 高刷原画质窗口 | `x=220, y=288`<br>`w=620, h=460`<br>`rx=58`<br>（中心 `530, 518`） | 线性渐变 `#F5F9FF`（42%）→ `#DCEBFF`（24%），内描边 `#FFFFFF`（45%，`6 px`） | Mode: `Combined`<br>Liquid Glass: `On`<br>Translucency: `75%`<br>Blur: `35%`<br>Specular: `Automatic`<br>Shadow: `Default` |
| **Group 3** | [`03-reticle.svg`](icon/layers/03-reticle.svg) | **光学精准准星**：四段 90° 断开圆弧 + 四向加粗圆角短刻度线，留白明确 | 中心 `(530, 518)`<br>圆环半径 `r=128`，线宽 `26`<br>刻度 `26×76`（`rx=13`），距心 `96–172`<br>外接 `344×344` | 纯白 `#FFFFFF`（96%） | Mode: `Combined`<br>Liquid Glass: `On`（若边缘过柔可降至低透明）<br>Translucency: `15%`<br>Specular: `Automatic`<br>Shadow: `Subtle` |
| **Group 4** | [`04-core-dot.svg`](icon/layers/04-core-dot.svg) | **系统蓝焦点核心**：正中心高亮实心圆点，呼应软件强调色 `#0A84FF` | 中心 `(530, 518)`<br>半径 `r=46`（直径 `92`） | 径向渐变 `#6CD4FF`（0%）→ `#0A84FF`（55%）→ `#005FD6`（100%） | Mode: `Combined`<br>Liquid Glass: `On`<br>Translucency: `10%`<br>Specular: `Automatic`<br>Shadow: `None` |

### 导入与验收步骤（Checklist）

1. 在 Xcode 附带的 **Icon Composer** 中新建 `.icon` 文档，平台勾选 **macOS**。
2. 在 **Background** 面板设置纵向线性渐变 `#0D1526` → `#162640`。
3. 依次将 `docs/handoff/gemini/icon/layers/` 下的 `01-source-screen.svg`、`02-main-screen.svg`、`03-reticle.svg`、`04-core-dot.svg` 拖入左侧 Sidebar（确认 `01` 在最底、`04` 在最顶，每层各成独立 Group）。
4. 按上表在右侧 Inspector 调整每组的 `Translucency`、`Blur`、`Specular` 与 `Shadow`。
5. 在预览面板逐一检查以下 **6 种外观 × 缩略尺寸**：
   - `Default` / `Dark`：确认后层蓝屏透过主屏玻璃产生柔和折射，且准星与中心蓝点锐利不发灰。
   - `Clear Light` / `Clear Dark`：若纯透明模式下前屏轮廓偏弱，可微调 Group 2 内描边或略增 Group 1 不透明度。
   - `Tinted Light` / `Tinted Dark`：确认准星与中心圆点仍能形成清晰明暗层次。
   - **16 px 实测**：在 Finder 列表视图 / Spotlight / Dock 最小尺寸下检查，确认“深底 + 横屏玻璃块 + 中心亮焦点”清晰可辨、无粘连。
6. 导出 `.icon` 文件接入工程（并按需为旧版 macOS 生成兼容 `.icns` 兜底）。

---

## 5. 定案补充（2026-11-25）：方向 B 双端统一方案（Mac 蓝 + 手机端极光翠绿）

用户确认选用 **方向 B「双屏跃迁与极速帧流」** 统一双端图形语言，仅通过配色区分桌面接收端与手机发送端：
- **Mac 端（GameScrcpy for macOS）**：浅冰蓝底 + 深海军蓝竖屏 + `#0A84FF` 系统蓝液态玻璃横屏 + 纯白折角（合层预览：[`icon/preview-b-macos.svg`](icon/preview-b-macos.svg)）。
- **手机端（Android / Companion）**：选用 **配色方案 1「极光翠绿（Emerald Signal）」**——浅薄荷白底 + 深松石墨绿竖屏 + `#10B981 → #34D399` 翡翠薄荷绿液态玻璃横屏 + 纯白折角（合层预览：[`icon/preview-b-android.svg`](icon/preview-b-android.svg)）。

### 5.1 方向 B · macOS Icon Composer 4 层拆解

依据 Apple [Creating your app icon using Icon Composer](https://developer.apple.com/documentation/xcode/creating-your-app-icon-using-icon-composer)，画布为 `1024×1024` 全出血正方形，现成 4 层 SVG 位于 [`icon/layers-b-macos/`](icon/layers-b-macos/)：

#### 背景填充（Background Inspector）
- **Default**：纵向线性渐变 `#F0F6FF`（顶）→ `#CFE4FF`（底）。
- **Dark**：可由系统自动适配，或手动指定深蓝渐变 `#0F1B2E` → `#172B47` 以增强暗色桌面下浅色前景的纵深。
- **Clear / Tinted**：交由系统自动着色。

#### 前景 4 组图层清单（由底至顶 Z 轴顺序）

| 组序 | 素材文件 | 内容说明 | 坐标与尺寸（1024 画布） | 基础色值 | Icon Composer 材质参数建议 |
|---|---|---|---|---|---|
| **Group 1** | [`01-phone-back.svg`](icon/layers-b-macos/01-phone-back.svg) | **后层竖屏手机**：偏左实心圆角竖屏，作为前方液态玻璃折射的深色底衬 | `x=186, y=196`<br>`w=304, h=632`<br>`rx=52` | 纵向线性渐变 `#1D3557` → `#0F2038` | Mode: `Combined`<br>Liquid Glass: `Off`（或低透明 `10%` 保持实心对比）<br>Shadow: `Subtle` |
| **Group 2** | [`02-glass-screen.svg`](icon/layers-b-macos/02-glass-screen.svg) | **前层系统蓝液态玻璃横屏**：居中偏右叠压手机右半区（重叠宽 `176 px`），产生强烈折射景深 | `x=314, y=304`<br>`w=556, h=416`<br>`rx=46` | 线性渐变 `#38BDF8`（88%）→ `#0A84FF`（92%）→ `#0066E6`（95%），内描边 `#FFFFFF`（38%，`5 px`） | Mode: `Combined`<br>Liquid Glass: `On`<br>Translucency: `65%`<br>Blur: `25%`<br>Specular: `Automatic`<br>Shadow: `Default` |
| **Group 3** | [`03-glass-sheen.svg`](icon/layers-b-macos/03-glass-sheen.svg) | **玻璃斜切高光折面（可选增强层）**：右上斜切半透明冷白光带，强化玻璃质感 | 裁切于前屏内：`points="738,304 870,304 870,720 622,720"` | 线性渐变 `#FFFFFF`（28%）→ `#FFFFFF`（4%） | Mode: `Combined`<br>Liquid Glass: `Off`<br>说明：若系统自带 Specular 已足够亮，可直接隐藏此层 |
| **Group 4** | [`04-chevron.svg`](icon/layers-b-macos/04-chevron.svg) | **中心纯白流光折角（`>`）**：位于前屏正中 `(592, 512)`，象征低延迟高速画面流 | `M 548 416 L 644 512 L 548 608`<br>线宽 `46 px`，圆角端点/转角 | 纯白 `#FFFFFF`（100%） | Mode: `Combined`<br>Liquid Glass: `On`<br>Translucency: `10%`<br>Specular: `Automatic`<br>Shadow: `Subtle` |

### 5.2 方向 B 手机端（极光翠绿）· Android Adaptive Icon 分层规范与色值

依据 Google 官方 [Adaptive icons — Android Developers](https://developer.android.com/develop/ui/compose/system/icon_design_adaptive) 规范：
- **画布尺寸**：全部图层统一为 **`108 × 108 dp`**（四周各预留 `18 dp` 用于 OEM 遮罩裁切与视差动效，不在图层内画圆角遮罩或外投影）。
- **安全区**：核心图形严格收束在中心 **`66 × 66 dp` 安全圆内（半径 `33 dp`，圆心 `(54, 54)`）**——经坐标验算，竖屏外角距圆心约 `31.7 dp`、前屏外角距圆心 `29.15 dp`，在圆形、圆角矩形、泪滴形等任意 Android 桌面遮罩下均不会裁边。
- **主题单色图标（Material You）**：同步提供 `monochrome` 单色图层，支持 Android 13+ 壁纸取色主题图标。

#### 极光翠绿（Emerald Signal）双端色值对照

| 元素 | Mac 端（方向 B · 系统蓝） | 手机端（方向 B · 极光翠绿） |
|---|---|---|
| **背景渐变** | `#F0F6FF` → `#CFE4FF` | `#ECFDF5` → `#C6F6E2` |
| **后层竖屏手机** | `#1D3557` → `#0F2038` | `#0F2E2A` → `#081C19` |
| **前层玻璃横屏** | `#38BDF8` → `#0A84FF` → `#0066E6` | `#34D399` → `#10B981` → `#059669` |
| **玻璃斜切光带** | `#FFFFFF`（28% → 4%） | `#FFFFFF`（26% → 4%） |
| **中心折角（`>`）** | `#FFFFFF`（100%） | `#FFFFFF`（100%） |

#### Android 分层素材清单（[`icon/layers-b-android/`](icon/layers-b-android/)）

| 图层类型 | 素材文件 | 内容与坐标（`108×108` viewBox） | 接入方式 |
|---|---|---|---|
| **Background** | [`ic_launcher_background.svg`](icon/layers-b-android/ic_launcher_background.svg) | 全出血 `108×108` 纵向线性渐变 `#ECFDF5` → `#C6F6E2` | 通过 Android Studio Vector Asset 转为 `res/drawable/ic_launcher_background.xml` |
| **Foreground** | [`ic_launcher_foreground.svg`](icon/layers-b-android/ic_launcher_foreground.svg) | 透明底；竖屏 `x=29, y=32, w=19, h=44, rx=3.8`；翡翠玻璃横屏 `x=38, y=39, w=41, h=30, rx=3.5`；纯白折角 `M 55 47 L 62 54 L 55 61`（线宽 `3.2`） | 转为 `res/drawable/ic_launcher_foreground.xml` |
| **Monochrome** | [`ic_launcher_monochrome.svg`](icon/layers-b-android/ic_launcher_monochrome.svg) | 纯单色镂空剪影：用 `1.8 dp` 负空间描边隔离竖屏与横屏，并将中心 `>` 折角镂空，确保单色主题下仍清晰可辨 | 转为 `res/drawable/ic_launcher_monochrome.xml` |

#### `res/mipmap-anydpi-v26/ic_launcher.xml` 配置参考

```xml
<?xml version="1.0" encoding="utf-8"?>
<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">
    <background android:drawable="@drawable/ic_launcher_background" />
    <foreground android:drawable="@drawable/ic_launcher_foreground" />
    <monochrome android:drawable="@drawable/ic_launcher_monochrome" />
</adaptive-icon>
```

> 注：旧版 Android（API < 26）传统图标及分发展示图标可直接由 [`icon/preview-b-android.svg`](icon/preview-b-android.svg) 导出 `512×512` 无透明通道 PNG。
