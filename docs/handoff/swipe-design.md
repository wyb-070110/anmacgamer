# 触控板滑动：照着安卓自己的规则来（2026-11-25）

结论：手机桌面和列表本来就有自己的惯性、翻页吸附和列表物理。电脑这边不再自己造惯性或翻页，只做一件事：**让手机看到一根真实手指的样子**，包括手指移动的真实节奏和离开屏幕那一刻的速度。剩下的交给手机。

## 查到的规则

**安卓 VelocityTracker**（`frameworks/native/libs/input/VelocityTracker.cpp`、`include/input/VelocityTracker.h`）：
- 坐标轴默认用 LSQ2（二次最小二乘），只看抬手前 **100 ms** 内的移动点（`HORIZON = 100 ms`，最多 20 个点）。
- **40 ms 停止规则**（`ASSUME_POINTER_STOPPED_TIME = 40ms`）：新点与上一个点相隔超过 40 ms，清空所有速度历史；`ACTION_UP` 离上一个点超过 40 ms，速度归零。
- `ACTION_UP` 的位置不算一个采样点（它只是“最后的位置”）；时间戳相同的点后者覆盖前者；位置相同、时间不同的点照常算进去（会把速度拉低）。
- 速度完全按 `MotionEvent.eventTime` 算。

**Launcher3 PagedView**（AOSP 桌面翻页，`PagedView.java`、`res/values/dimens.xml`）：
- 抬手时：速度超过 `fling_threshold_velocity = 500dp/s`（拖动已过 slop）算甩动，目标页是 `mCurrentPage ± 1`，**一次甩动只翻一页**；拖动中 `mCurrentPage` 不变。
- 没甩动但拖过页宽 40%（`SIGNIFICANT_MOVE_THRESHOLD = 0.4`）也翻；否则吸附到离屏幕中心最近的页。
- 吸附动画：速度低于 `min_fling_velocity = 250dp/s` 用默认时长（`config_pageSnapAnimationDuration = 750`）；否则按速度算，最低 `min_page_snap_velocity = 1500dp/s`。

**三星 One UI Home**：闭源，没找到技术文档；社区帖子里也没有“甩动翻多页”的官方说明。按 Launcher3 的规则推断，一次滑动翻两页最可能是手机算出的速度远高于手指真实速度（见下）。

## 我们哪里不像真手指

1. **时间戳**：手机原来按“收到的时刻”给触摸打时间，USB/adb 成批送达时几个点挤在一起，速度被算大几倍 → 甩过头、跳页。**已改**（2f3fcb2）：每个触摸帧带电脑的时间，手机按电脑的节奏打时间。
2. **抬手**：触控板的“手指离开”事件通常没有位移，我们原来还是发了一个原地移动再抬起；按上面的规则，这个原地点会把速度拉低，若晚于 40 ms 还会让速度归零（快甩翻不动）。**已改**：没有位移就不发移动；手指离开时若还在动（最近 40 ms 内有移动），按最近 100 ms 的速度再走两步（各 8 ms）然后立刻抬起，和真手指离开玻璃时一样；已经停住就原地抬起，不甩。
3. **默认方式**：默认改为“跟手”（1:1 映射），不再用电脑这边的翻页、惯性和单次上限；macOS 的惯性（momentum）事件一律不发给手机，惯性由手机自己算。

## 怎么验证

- 退出重开 GameScrcpy（新的手机端会在连接时自动装上）。
- 桌面：慢拖过半松手 → 翻一页；快甩 → 翻一页，不跳两页；拖一点停住再松手 → 弹回。
- 列表、应用抽屉：快甩有手机自己的惯性，慢拖松手就停。
- 需要数据时：`GS_TOUCH_TRACE=/tmp/touch.log` 记录发出的触摸帧（见 `src/app/ControlChannel.cpp`）。

来源：AOSP `frameworks/native`（VelocityTracker.cpp / VelocityTracker.h）、`packages/apps/Launcher3`（PagedView.java、res/values/dimens.xml、config.xml），android.googlesource.com，main 分支；Gemini 的 `docs/handoff/gemini/research-swipe.md`。

## 结果（2026-11-25 下午，用户实测）

- 左右翻页：完美（时间戳 + 松手时按速度再走两步）。
- 上下：录到每次滑完约 9 ms 后，macOS 的第一个惯性事件以“开始”相位带着位移到来，被当成新的一次滑动，在指针处按下一根手指，打断手机的惯性滚动（上下卡顿）。已丢弃这种事件（松手后 150 ms 内带位移的开始）。上下松手改回最初版本：直接抬起，由手机按真实移动算速度。用户实测：刷抖音“完美”。
