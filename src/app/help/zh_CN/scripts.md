方案逻辑允许使用 TypeScript、JavaScript 或可视化蓝图在方案启用时自动运行——发送触控、等待画面变化、识图找色，并在投屏窗口弹出提示。

## 第一个脚本

1. **打开逻辑**：编辑方案时，将顶栏中央从**画面**切到**逻辑**（或在投屏窗口**更多 > 编辑逻辑**、方案卡片**更多 > 编辑逻辑…**中打开）。
2. **添加逻辑**：点击**添加逻辑**。anmacgamer 会把单文件 `.json` 方案转为包含 `profile.json`、`logic.ts` 和 `tsconfig.json` 的方案文件夹（扩展方案需先复制到我的方案）。
3. **编写或连线**：在**代码**视图编辑 `logic.ts`，或切到右上角**蓝图**拖拽连线。坐标采用屏幕比例，左上角为 `(0, 0)`，右下角为 `(1, 1)`。
4. **运行**：点击**保存**（或右侧**运行**），投屏中的手机无需重连即可热重载生效。

![逻辑视图](figure:script-workbench)

| 区域 | 作用 |
|---|---|
| :folder-open: 文件 | 浏览和新建方案文件夹内的文件。 |
| :code-xml: 编辑器 | 编写 TypeScript 或**蓝图**；输入 `touch.` 或 `controls.` 获取补全。 |
| :play: 运行与接口 | 查看运行状态、点击**运行**重载，或搜索 **API** 文档。 |
| :terminal: 控制台与问题 | 查看 `log.info` 输出并点击报错定位。 |

> [!TIP]
> 习惯用 VS Code？在顶栏选择**更多 > 在外部编辑器中打开**，外部保存后会自动同步。

---

## 参考：按键控制与事件

`profile.json` 中的每个按键都对应 `controls.<id>`（带短横线的 id 用中括号，如 `controls["dash-move"]`）。

```ts title="logic.ts"
import { controls, touch, log } from "gamescrcpy";

controls.fire.onPress(async () => {
    log.info("fire pressed");
});

controls.jump.whileHeld(async ({ signal }) => {
    while (!signal.aborted) {
        await touch.tap(0.5, 0.8);
        await sleep(100);
    }
});
```

- **按键事件**：`onPress`、`onRelease` 和 `whileHeld`（循环内务必使用 `await`，避免阻塞逻辑）。
- **主动调用**：`trigger()`、`press()`、`hold()`、`enable()`、`disable()`、`toggle()`。
- **按键事件 vs 触控事件**：**按下时**监听键盘鼠标按键；**开始触控/结束触控**监听屏幕上的手指动作（建议监听事件而非直接读取 `touching`）。
- **接管按键**：对点击、按住、长按、连发、滑动、多点按住监听 `onPress`/`onRelease`/`whileHeld` 时，按键会交由逻辑接管。可在逻辑中调用 `controls.<id>.press()` / `.tap()`，或传入 `{ consume: false }`（蓝图设为**照常按屏幕，再运行逻辑**）让组件先按屏幕。摇杆、视角、技能以及任务、扩展和 AI 脚本仅监听不拦截。
- **状态绑定**：`controls.fire.bind("enabled", () => !vars.boat.value);` 会在依赖状态变化时自动刷新启用状态（手动赋值、调用 `mapping.setEnabled` 或任务结束时解除）。
- **界面与窗口**：`ui.toast("Round 2")`、`ui.mark(0.5, 0.8, { label: "here" })`（持续到 `dispose()` 或任务结束）、[`clipboard.read()`](help:api/clipboard) / `clipboard.write(text)`（读写手机剪贴板，不可用时返回 `null`/`false`）、`window.focused()`、`window.onFocus`、`window.onBlur`。

## 参考：等待画面变化

在任意动作（`touch.tap`、`swipe`、`controls.fire.trigger()` 等）后链式调用 `.until`，每帧检测一次条件：

![等待画面变化](figure:until-flow)

```ts title="logic.ts"
const button = await touch.tap(0.9, 0.9)
    .until(() => vision.find("Start button"), { timeoutMs: 3000, retry: 2 });
```

- 条件返回真值时立即结束等待并返回该结果。
- 超出 `timeoutMs`（默认 `5000` 毫秒）未满足条件时重试动作，最多重试 `retry` 次；全部失败后抛出 `TimeoutError`。

## 参考：蓝图与流程控制

**蓝图**直接映射 TypeScript 代码：`logic.ts` 为主页面，各组件逻辑位于 `controls/<code name>.ts`，公共函数位于 `library.ts`（`export function`）。

- **在线上插入节点**：将游离块拖到高亮连线上、右键连线选**在此插入块…**，或从已有连线的执行引脚拖到空白处。
- **并行执行**：从已有连线的执行输出引脚再拖出第二（或第三）根线，会自动生成 `await parallel(async () => { … }, async () => { … });`；按住 Option 点击引脚可清除全部连线。
- **内置流程**：`switch`（**按值分支**）、`await sequence(...)`、`await multiGate("gate", 3, false, false, ...)`、`doN("name", 3)` / `resetDoN("name")`、`for` / `for (const [index, item] of list.entries())`。
- **集合、文本与数学**：`list`（`add`、`insert`、`removeAt`、`remove`、`indexOf`、`at`、`setAt`、`size`、`has`、`sort`、`shuffle`、`clear`）、`dict`（`make`、`set`、`get`、`has`、`remove`、`keys`、`values`、`size`、`clear`）、`set`（`make`、`add`、`has`、`remove`、`values`、`size`、`clear`）、`text`（`join`、`format`、`find`、`has`、`split`、`replace`、`upper`、`lower`、`trim`、`length`）、`math`（`round`、`floor`、`ceil`、`abs`、`min`、`max`、`clamp`、`lerp`、`random`、`randomInt`）。

## 参考：变量、结构体、枚举与类

文件顶层的 `let` 声明会显示为蓝图变量（支持 `boolean`、`number`、`string`、`PicturePoint`、`Region`、`HexColor`、`ImagePath`、`InputName`、`Component`、数组 `[]`、`Map<K, V>`、`Set<T>`）：

```ts title="logic.ts"
/** Health of the hero. @category Battle */
let health: number = 100;
let best: number = (store.get("best") as number) ?? 0; // @keep
```

- JSDoc 注释用作悬停提示；`@category` 分组；`@duration` 标记毫秒时长；`@keep` 通过 `store.get`/`store.set` 跨次保留。
- 将变量拖到画布可选**获取**（<kbd>⌘ Command</kbd>+拖拽）或**设置**（<kbd>⌥ Option</kbd>+拖拽）；右键输入引脚可选**提升为变量**。
- **结构体**：`interface 位置 { x: number; y: number; }`（蓝图 **生成** / **拆分**）。
- **枚举**：`const 阵营 = { 红: "红", 蓝: "蓝" } as const; type 阵营 = typeof 阵营[keyof typeof 阵营];`（蓝图 **按枚举分支**）。
- **类**：`class 敌人 { ... }` 支持 `constructor()`、`private` 字段、`this.event()` 分发器（**触发** / **绑定** / **解绑**）、继承（`super.method()` 对应**调用父类**）、**新建**和**类型转换**（`instanceof`）。

## 参考：自定义组件

在 `logic.ts` 继承 `Component`，或在键位编辑器组件库点击**新建组件…**保存为可复用组件（`components/<folder>/component.ts` + `component.json`）：

```ts
export default class HoldToggle extends Component {
    static info = { name: "Hold toggle" };
    static props = { key: prop.key("F"), at: prop.position(point(0.5, 0.8)) };
    pressed = this.event();
    released = this.event();
    holding = false;

    setup() {
        this.props.key.onPress(() => {
            this.holding = !this.holding;
            if (this.holding) this.finger.down(this.props.at);
            else this.finger.up();
            (this.holding ? this.pressed : this.released).emit();
        });
    }
}
```

- `static props` 支持 `prop.key`、`prop.position`、`prop.number`、`prop.text`、`prop.toggle`、`prop.component`。
- `this.finger` 持有独立触控手指直到 `up()` 或逻辑停止；`this.position` 为归一化放置坐标。
- **扩展内置组件**：继承 `Tap`、`Hold`、`LongPress`、`Turbo`、`Swipe`、`Pinch`、`Wasd`、`RightWalk`、`PadStick`、`Camera`、`PadCamera`、`FreeLook` 或 `Skill`（如重写 `onPressed()` 会在原生触控之外额外执行）。
- **子组件与接口**：`this.child(BuiltInClass)` 可成组放置子组件；`defineInterface<{ fire(): void }>("fire")` 配合 `static implements = [Fire]` 可通过 `Fire.all()` 或 `mapping.all(Fire)` 批量调用。

## 参考：自动化任务

任务独立于方案运行（适合每日签到或挂机），切换游戏不中断且不抢占手动按键。在**任务**页新建任务（`task.ts`）：

```ts
export default class Farm extends Task {
    static params = { rounds: param.number(10, { label: "Rounds", min: 1 }) };

    async run({ signal }: TaskContext) {
        for (let round = 1; round <= this.params.rounds && !signal.aborted; round++) {
            this.progress("Round " + round);
            await touch.tap(0.5, 0.8);
            await sleep(5000);
        }
    }
}
```

- `static params`（`param.number`、`param.text`、`param.toggle`）会生成右侧配置项；`this.progress(...)` 更新任务卡片与投屏工具栏进度。
- 支持每日、每周或单次定时启动，勾选运行设备，并可开启失败重试。

## 参考：视觉工作台

在投屏窗口**更多 > 视觉工作台**（或方案卡片**更多 > 视觉资源…**）中制作保存在 `res/` 的识图资源：

1. 点击**定格**暂停当前画面。
2. 用工具栏框选模板、区域、点位、路径或取色。
3. 在右侧命名并调整相似度阈值、彩色/灰度匹配与搜索区域。
4. 点击**测试查找**验证匹配结果和耗时。

![视觉工作台](figure:vision-freeze,vision-draw,vision-name,vision-test!)

在代码里按名称调用：

```ts title="logic.ts"
const start = await vision.find("Start button");
const banner = await vision.findColor("Event banner");
const entry = res.point("Event entry");
await touch.tap(entry.x, entry.y);
const pixel = vision.pickColor(point(0.5, 0.1)); // "#RRGGBB"
const ok = vision.matchColor(point(0.5, 0.1), "Event banner");
```

当 AI 客户端通过 MCP 请求缺失的视觉资源时，工作台会自动打开待补充列表，框选后点**交给 AI**（不需要的可点**跳过**）。

## 参考：命令行工具（`gs`）与日志

`gs` 位于 `anmacgamer.app/Contents/MacOS/`。`log.info`、`log.warn`、`log.error` 的输出同时保存在 `profiles` 同级的 `logs` 文件夹中。

| 命令 | 作用 |
|---|---|
| `./gs check <folder>` | 检查逻辑类型与语法错误。 |
| `./gs types <folder>` | 修改 `profile.json` 或 `res/` 后重新生成类型定义。 |
| `./gs run <folder> --replay <session.json>` | 用录制会话回放并输出逻辑执行结果。 |
| `./gs test <folder>` | 运行 `<folder>/tests` 下的测试。 |
| `./gs new <new folder> --profile <profile.json>` | 从导出的方案生成带逻辑的方案文件夹。 |

## 脚本接口

- [脚本 API 总览](help:api)
- [touch](help:api/touch)：点击与滑动
- [mapping](help:api/mapping)：当前方案按键
- [ui](help:api/ui)：投屏窗口提示与弹窗
- [window](help:api/window)：投屏窗口焦点状态
- [vision](help:api/vision)：找图与找色
- [res](help:api/res)：视觉工作台命名的点、区域与路径
- [store](help:api/store)：跨次运行持久化数据
