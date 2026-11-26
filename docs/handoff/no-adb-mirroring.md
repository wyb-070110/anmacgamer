# 无 adb 投屏（设计，2026-11-25）

用户要的是：手机不开 USB 调试也能看到画面、听到声音，并用键鼠操作。现在没有 adb 时只有“无画面控制窗口”
（AOA 触控 + 伴侣扫码），用户明确说不需要那个，要真正的投屏。

## 结论

**让伴侣 App 在手机上当一个“服务端”**：用系统录屏（MediaProjection）把屏幕送进硬件编码器，把编码后的画面、
声音按**现有协议**（`protocol/SPEC.md` 第 2～3 章：control / video / audio 通道、媒体包）发给电脑。电脑的会话层
本来就按传输抽象（`src/session/include/gs/session/Transport.h`：`Transport` → `Link` → `FrameChannel` /
`MediaChannel`，已有 AdbTunnel、WifiDirect、Extension 三种），只要多一种传输，解码、显示、录像、键位、逻辑全都不用改。
触控继续走 AOA（数据线上的虚拟触摸屏，已经能用，见 c610034），“画面走会话 + 触控走 AOA”这种组合电脑上已经有
（“USB · 画面 + AOA 触控”）。

顺序：**先 Wi-Fi，再 USB**。伴侣已经有扫码配对、加密的局域网连接（SPEC 11.7），整条链路最快能跑通、能测；
USB 配件通道只是换一根“管子”，录屏和编码不变。

## 系统限制（安卓规定，绕不过去）

- 每次连接手机都会弹“开始录制或投放？”，要点“开始”；安卓 14 起还要选“整个屏幕 / 单个应用”。
- 设了 `FLAG_SECURE` 的应用（银行、部分游戏、版权视频）录出来是黑的。
- 声音用 `AudioPlaybackCapture`（安卓 10+，要录屏授权）；应用可以声明不让录（`allowAudioPlaybackCapture=false`），
  通话声音录不到。
- 录屏期间状态栏有录屏图标，前台服务类型 `mediaProjection`（伴侣截图功能已经在用，SPEC 11.8）。

## 链路

### 复用的协议

通道内的内容与 adb 隧道完全相同：channel 0 是 control（`ServerHello` → `ClientHello` → `Welcome`，之后控制消息），
1 是 video、2 是 audio（`ChannelOpen` → `ChannelAccepted` → 媒体包，SPEC 3.2）。伴侣在 `Welcome` 里只报它做得到的能力：
没有注入触控（触控走 AOA），没有剪贴板、启动应用、息屏等需要 shell 权限的功能（以后可以用伴侣自己的权限补一部分）。

### 新的一层：复用流（mirror link）

一条字节流上跑多个通道：记录 = `channel u8 · length u32 · payload bytes[length]`（payload 就是那个通道原本的字节）。
- **Wi-Fi**：伴侣按 SPEC 11.7 的方式另开一条 TCP 连到电脑的同一个端口，开头用新的 magic `GSCM`（mirror），握手与密钥
  派生同 11.7（上下文字符串换成 `"GSCP/1 companion mirror"`），之后每条复用记录放进一条 AES-256-GCM 加密记录；
  加密记录上限从 16 KiB 放宽到 1 MiB（视频关键帧较大，超过的按复用记录切开）。
- **USB（第二步）**：手机进入 AOA 配件模式，伴侣用 `UsbManager.openAccessory` 拿到读写流，电脑用 libusb 的 bulk 端点；
  同样的复用记录，不加密（物理连接）。第一次手机会问“用 GameScrcpy 伴侣打开这个 USB 配件？”，勾“始终”。

## 分步

1. **安卓：不抽共享模块**（看过了：server 的编码类和 shell 权限的部分——`DisplayMirror`、`EncoderCatalog`、`MediaClock`——
   缠在一起，搬出来风险大）。伴侣写一条精简的管线（录屏 → MediaCodec → 媒体包；声音 → Opus → 媒体包），只共用
   `:protocol` 模块（`Media`、`Messages`、`Proofs` 等编解码），server 一行不动。
2. **安卓：伴侣的投屏服务** `MirrorServer`：录屏授权 → `VirtualDisplay` 画到编码器的输入 Surface → 媒体包；
   `AudioPlaybackCapture` → Opus → 媒体包；按上面的协议应答 control 通道；方向变化时重建编码会话（新会话号）。
3. **协议与金标准向量**：SPEC 新增“11.10 伴侣投屏”（`GSCM` 握手、复用记录、能力子集），`protocol/tools/gen_vectors.py`
   生成向量，两端测试都跑。
4. **电脑：`CompanionMirrorTransport`**（`TransportKind::Companion`）：在伴侣局域网监听里接受 `GSCM` 连接，解复用成
   `FrameChannel` / `MediaChannel`；DeviceManager：没有 adb、伴侣已配对并连着时，点“投屏”开这种会话，触控照旧走 AOA。
   手机卡片显示“无线 · 伴侣画面 + USB · AOA 触控”。
5. **和用户一起测**（Wi-Fi）：画面、声音、旋转、延迟（电脑上的端到端延迟测量照常可用）、断线重连。
6. **USB 配件通道**：电脑 `Usb.mm` 增加切换配件模式与 bulk 读写；伴侣声明 `usb-accessory` 过滤并接管；同一复用记录。

## 风险

- 录屏编码的延迟比 shell 下的 `SurfaceControl` 镜像略高（多一层 VirtualDisplay），一般在 1 帧以内。
- 部分三星机型录屏最高 60 fps。
- 伴侣进程被系统杀掉时投屏中断：前台服务 + 通知保活，电脑按现有的“重连”逻辑处理。
