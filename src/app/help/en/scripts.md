Logic lets a profile run TypeScript, JavaScript, or visual blueprints while active—sending touches, waiting for screen changes, matching images or colors, and showing prompts in the mirror window.

## Your first script

1. **Open Logic**: while editing a profile, switch the top bar from **Picture** to **Logic** (or select **Edit logic** from the mirror **More** menu or profile card).
2. **Add logic**: click **Add logic**. anmacgamer converts a single `.json` profile into a profile folder with `profile.json`, `logic.ts`, and `tsconfig.json`. (Copy extension profiles to your own profiles first.)
3. **Write or build**: edit `logic.ts` in **Code** view, or switch to **Blueprint** at the top right to connect visual blocks. Positions use screen fractions from `(0, 0)` top-left to `(1, 1)` bottom-right.
4. **Run**: click **Save** (or **Run** on the right panel) to reload logic live on the mirrored phone without reconnecting.

![The logic view](figure:script-workbench)

| Panel | Purpose |
|---|---|
| :folder-open: Files | Browse and add files in the profile folder. |
| :code-xml: Editor | Edit TypeScript or **Blueprint**; type `touch.` or `controls.` for autocomplete. |
| :play: Run & API | Check status, click **Run** to reload, or search the **API** reference. |
| :terminal: Console & problems | View `log.info` output and jump to errors. |

> [!TIP]
> Prefer VS Code? Choose **More > Open in external editor** on the top bar. External edits reload automatically.

---

## Reference: Controls & events

Each key in `profile.json` is accessed via `controls.<id>` (use `controls["dash-move"]` for ids with dashes).

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

- **Key events**: `onPress`, `onRelease`, and `whileHeld` (always `await` inside loops so logic does not stall).
- **Direct calls**: `trigger()`, `press()`, `hold()`, `enable()`, `disable()`, `toggle()`.
- **Touch vs key events**: **When pressed** listens to the key; **When touching starts/ends** listens to the finger on screen (prefer touch events over reading `touching`).
- **Consuming keys**: listening to `onPress`/`onRelease`/`whileHeld` on Tap, Hold, Long press, Turbo, Swipe, or Multi-press claims the key so only logic runs. Call `controls.<id>.press()` / `.tap()` to press, or pass `{ consume: false }` (in Blueprint: **Press it as usual, then run the logic**) to let the component press first. Sticks, cameras, skills, tasks, extension scripts, and AI scripts only listen.
- **State binding**: `controls.fire.bind("enabled", () => !vars.boat.value);` updates automatically when tracked state changes (cleared if set manually or via `mapping.setEnabled`, or when a task ends).
- **UI & window**: `ui.toast("Round 2")`, `ui.mark(0.5, 0.8, { label: "here" })` (until `dispose()` or task end), [`clipboard.read()`](help:api/clipboard) / `clipboard.write(text)` (phone clipboard; returns `null`/`false` when unavailable), `window.focused()`, `window.onFocus`, `window.onBlur`.

## Reference: Wait until the screen changes

Chain `.until` onto any action (`touch.tap`, `swipe`, `controls.fire.trigger()`, etc.) to wait for a condition checked once per frame:

![Wait until the picture changes](figure:until-flow)

```ts title="logic.ts"
const button = await touch.tap(0.9, 0.9)
    .until(() => vision.find("Start button"), { timeoutMs: 3000, retry: 2 });
```

- Returns the condition's truthy result once met.
- Retries the action up to `retry` times if `timeoutMs` (default `5000`) expires; throws `TimeoutError` if all retries fail.

## Reference: Blueprints & flow

**Blueprint** mode represents TypeScript directly: `logic.ts` is the main page, each component has `controls/<code name>.ts`, and shared functions live in `library.ts` (`export function`).

- **Insert on wire**: drag a loose block onto a lit wire, right-click a wire (**Insert a block here…**), or drag from a wired exec pin onto empty canvas.
- **Parallel**: drag a second (or third) wire from an exec output that already has one to generate `await parallel(async () => { … }, async () => { … });`. Option-click the output to clear all wires.
- **Built-in flow**: `switch` (**Switch on Value**), `await sequence(...)`, `await multiGate("gate", 3, false, false, ...)`, `doN("name", 3)` / `resetDoN("name")`, `for` / `for (const [index, item] of list.entries())`.
- **Collections, text & math**: `list` (`add`, `insert`, `removeAt`, `remove`, `indexOf`, `at`, `setAt`, `size`, `has`, `sort`, `shuffle`, `clear`), `dict` (`make`, `set`, `get`, `has`, `remove`, `keys`, `values`, `size`, `clear`), `set` (`make`, `add`, `has`, `remove`, `values`, `size`, `clear`), `text` (`join`, `format`, `find`, `has`, `split`, `replace`, `upper`, `lower`, `trim`, `length`), `math` (`round`, `floor`, `ceil`, `abs`, `min`, `max`, `clamp`, `lerp`, `random`, `randomInt`).

## Reference: Variables, structs, enums & classes

Top-level `let` declarations become blueprint variables (`boolean`, `number`, `string`, `PicturePoint`, `Region`, `HexColor`, `ImagePath`, `InputName`, `Component`, arrays `[]`, `Map<K, V>`, `Set<T>`):

```ts title="logic.ts"
/** Health of the hero. @category Battle */
let health: number = 100;
let best: number = (store.get("best") as number) ?? 0; // @keep
```

- JSDoc comments set tooltips; `@category` groups variables; `@duration` marks milliseconds; `@keep` persists via `store.get`/`store.set`.
- Drag a variable onto the canvas for **Get** (<kbd>⌘ Command</kbd>-drag) or **Set** (<kbd>⌥ Option</kbd>-drag), or right-click an input pin and choose **Promote to variable**.
- **Structures**: `interface 位置 { x: number; y: number; }` (Blueprint **Make** / **Break**).
- **Enums**: `const 阵营 = { 红: "红", 蓝: "蓝" } as const; type 阵营 = typeof 阵营[keyof typeof 阵营];` (Blueprint **Switch on**).
- **Classes**: `class 敌人 { ... }` with `constructor()`, `private` fields, `this.event()` dispatchers (**Fire** / **Bind** / **Unbind**), inheritance (`super.method()` via **Call parent**), **New**, and **Cast to** (`instanceof`).

## Reference: Custom components

Extend `Component` in `logic.ts` or create a reusable library component via **New component…** in the keymap editor (`components/<folder>/component.ts` + `component.json`):

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

- `static props` supports `prop.key`, `prop.position`, `prop.number`, `prop.text`, `prop.toggle`, `prop.component`.
- `this.finger` holds a dedicated touch until `up()` or logic stop; `this.position` holds normalized placement ratios.
- **Extend built-in components**: subclass `Tap`, `Hold`, `LongPress`, `Turbo`, `Swipe`, `Pinch`, `Wasd`, `RightWalk`, `PadStick`, `Camera`, `PadCamera`, `FreeLook`, or `Skill` (e.g., `onPressed()` runs alongside native touch).
- **Children & interfaces**: `this.child(BuiltInClass)` places grouped child components; `defineInterface<{ fire(): void }>("fire")` with `static implements = [Fire]` lets logic call `Fire.all()` or `mapping.all(Fire)`.

## Reference: Tasks

Tasks run independently of profiles (for daily check-ins or farming) while you keep playing. Create one on the **Tasks** page (`task.ts`):

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

- `static params` (`param.number`, `param.text`, `param.toggle`) renders settings on the **Tasks** page; `this.progress(...)` updates the task card and mirror toolbar.
- Schedule tasks daily, weekly, or once, select target phones, and optionally retry on failure.

## Reference: Vision workbench

Open **More > Vision workbench** in the mirror window (or **Vision resources…** on a profile card) to save templates, regions, points, paths, and colors in `res/`:

1. Click **Frozen** to pause the frame.
2. Draw a template, region, point, path, or pick a color.
3. Name it and tune similarity threshold, color/grey mode, and search region.
4. Click **Test find** to check matches and speed.

![The vision workbench](figure:vision-freeze,vision-draw,vision-name,vision-test!)

Use saved resources by name in code:

```ts title="logic.ts"
const start = await vision.find("Start button");
const banner = await vision.findColor("Event banner");
const entry = res.point("Event entry");
await touch.tap(entry.x, entry.y);
const pixel = vision.pickColor(point(0.5, 0.1)); // "#RRGGBB"
const ok = vision.matchColor(point(0.5, 0.1), "Event banner");
```

If an AI client requests missing vision items over MCP, the workbench opens with the AI's requested list so you can capture or **Skip** them and click **Give to AI**.

## Reference: CLI tool (`gs`) & logs

`gs` lives inside `anmacgamer.app/Contents/MacOS/`. Logs from `log.info`, `log.warn`, and `log.error` are written to the `logs` folder next to `profiles`.

| Command | Action |
|---|---|
| `./gs check <folder>` | Type-check and validate logic. |
| `./gs types <folder>` | Regenerate type definitions after editing `profile.json` or `res/`. |
| `./gs run <folder> --replay <session.json>` | Replay a recorded session and print logic output. |
| `./gs test <folder>` | Run tests in `<folder>/tests`. |
| `./gs new <new folder> --profile <profile.json>` | Create a logic profile folder from an exported profile. |

## Script API

- [Script API overview](help:api)
- [touch](help:api/touch): taps and swipes
- [mapping](help:api/mapping): active profile keys
- [ui](help:api/ui): toasts, prompts, and overlays
- [window](help:api/window): mirror window focus state
- [vision](help:api/vision): image and color matching
- [res](help:api/res): named points, regions, and paths
- [store](help:api/store): persistent values across runs
