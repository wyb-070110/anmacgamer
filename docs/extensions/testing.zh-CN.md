# 测试扩展

[English](testing.md) · 简体中文

无需打开 anmacgamer 或连接手机，可用程序旁的 `gs.exe` 完成校验、类型检查与测试。

| 命令 | 用途 |
|:--|:--|
| `gs validate <文件夹 \| 包.gsext \| 整合包.gsbundle> [--json]` | 检查清单、文件、滤镜着色器、方案、签名与 DLL 架构/`gs_ext_main` |
| `gs check <文件夹> [--json]` | TypeScript 类型检查与脚本规则检查 |
| `gs types <文件夹>` | 生成 `types/gamescrcpy.d.ts` 与 `types/extensions.d.ts` 供编辑器使用 |
| `gs ext test <文件夹 \| 包.gsext> [--test <文件>]... [--hosted] [--json]` | 在模拟宿主中加载清单、DLL 与脚本并运行 `tests/*.json` |
| `gs ext init <native \| script \| theme> <文件夹> [--id <id>]` | 生成带测试的最小扩展模板 |
| `gs ext <id \| 文件夹 \| 包.gsext> <命令> [--<参数> <值>]... [--hosted] [--json]` | 运行 `contributes.cli` 声明的命令 |

## `gs validate`

```powershell
gs validate .\my-extension
gs validate .\alice.lap-timer-1.2.0.gsext --json
```

执行与安装器相同的检查：清单字段、路径、大小、滤镜着色器编译（`--compile-timeout <秒>`，默认 20 秒）、方案与补丁、签名，以及 `contributes.native.dll` 中每个 DLL 的架构和 `gs_ext_main` 导出。退出码：`0` 通过，`1` 有错误，`2` 命令行参数错误。

## `gs check` 与 `gs types`

- `gs check <文件夹>` 用自带 TypeScript 编译器检查所有 `.ts`（跳过 `types/`、`node_modules` 与点开头目录），禁止无法擦除的语法（`enum`、装饰器）及不安全写法（无 `await` 死循环、找图前固定 `sleep`）。需要 `tsconfig.json` 和 `src/main.ts`（或 `src/main.js`），通过时输出 `no problems`。
- `gs types <文件夹>` 生成 `types/gamescrcpy.d.ts` 与 `types/extensions.d.ts`（已装原生扩展的节点）。anmacgamer 更新后请重跑。

## 模拟宿主：`gs ext test`

```powershell
gs ext test .\my-extension
gs ext test .\my-extension --hosted
gs ext test .\alice.lap-timer-1.2.0.gsext --test .\more-tests\flash.json --json
```

执行流程：
1. 先按 `gs validate` 校验，有错则以退出码 `2` 中止。
2. 对假手机加载每个脚本 2 秒测试时间（记录 `ui.*` 调用；用到未模拟服务的标为 `limited`；脚本报错标为 `failed`）。
3. 默认进程内加载 DLL，加 `--hosted` 则在 `gs-exthost.exe` 中加载。`"sandbox": "restricted"` 的 DLL 始终在 `gs-exthost.exe` 的临时 AppContainer（`gs-test:<id>`）中运行。
4. 按文件名顺序运行 `tests/*.json`（或 `--test` 指定文件），每次测试重置 DLL、默认设置与全部清单权限。

退出码：`0` 全通过，`1` 有失败或无测试，`2` 无法加载。

源码构建测试受限 DLL 提示：Administrators 属主的 `gs-exthost.exe` 在 AppContainer 下可能报错误 5，请将 `gs.exe`、`gs-exthost.exe` 及旁边 DLL 复制到用户自有目录再运行。

### 测试文件

UTF-8 JSON，最大 1 MiB、嵌套不超过 64 层：

```json
{
  "format": "gamescrcpy.ext-test",
  "version": 1,
  "name": "flashing",
  "granted": ["hardware"],
  "settings": { "speed": 115200 },
  "deviceSettings": { "mock-phone": { "port": "COM7" } },
  "holders": [ { "resource": "serial:COM9", "by": "AOA session", "yields": true } ],
  "timeout": 20000,
  "steps": [
    { "do": "call", "node": "version", "expect": { "value": "1.0.2" } },
    { "do": "job", "job": "flash", "args": { "port": "COM9" },
      "expect": { "result": { "written": true }, "progress": [ { "text": "erasing" }, { "fraction": 1 } ] } }
  ]
}
```

| 字段 | 含义 |
|:--|:--|
| `format`、`version` | 可选；填写时须为 `gamescrcpy.ext-test` 与 `1` |
| `name` | 报告名称（默认文件名） |
| `granted` | 本测试授予的清单权限子集（默认全部） |
| `settings`、`deviceSettings` | 初始设置；假手机 id 为 `mock-phone` |
| `holders` | 初始已占用硬件的持有者：`{ resource, by, yields }` |
| `timeout` | 单步等待毫秒数（1–600000，默认 10000） |
| `steps` | 1–1000 步（`do`，可选 `name`、`expect`、单步 `timeout`） |

### 步骤

| `do` | 输入 | `expect` 可检字段 |
|:--|:--|:--|
| `call` | `node`、`args`（数组） | `value`、`error` |
| `job` | `job`、`args`（对象）、`id`、`wait`（默认 `true`） | `result`、`error`、`progress`、`logs`、`outputs` |
| `job.cancel` / `job.wait` | `id` | `ended` / `result`、`error`、`progress`、`logs`、`outputs` |
| `touch.open` | `method`、`session`、`phone`、`device`、`touchBox`、`options` | `opened` |
| `touch.frame` | `session`、`contacts`（`{ id, action, x, y, … }`，`action`：`down`/`move`/`up`/`cancel`）、`rotation` | `ok`、`sent` |
| `touch.key` | `session`、`key` 或 `keycode`、`down`、`repeat`、`meta` | `ok` |
| `touch.text` | `session`、`text` | `ok` |
| `touch.mouse` | `session`、`buttons`、`dx`、`dy`、`wheel` | `ok` |
| `touch.pad` | `session`、`buttons`、`hat`、`leftX`、`leftY`、`rightX`、`rightY`、`leftTrigger`、`rightTrigger` | `ok` |
| `touch.close` | `session` | — |
| `devices` | `provider` | `devices`、`count`、`error`、`problems` |
| `device.act` | `provider`、`device`、`action` | `ok`、`error` |
| `events` | — | 输出上次 `events` 之后的事件 |
| `settings` | `settings`、`deviceSettings` | `changed`（传给 `settings_changed` 的键） |
| `wait` | `ms` | 推进测试时钟（含进程内 `now_us`） |
| `holder` / `holder.release` | `resource`、`by`、`yields` / `resource` | — / `released` |
| `claim.want` / `claims` | `resource`、`by` / — | `held`、`yielded`、`released` / `held`、`holders` |
| `scripts` / `manifest` | — | `loaded`、`logs` / `value` |

每步还可按顺序检查 `events`（`{ topic, value }`）、`logged`（`{ level, text }` 或子串）、`notified`（`{ title, text }` 或子串）与 `devicesChanged`（提供者名）。比较规则：对象只比列出的键；数组比长度和逐项；数字允许极小误差；`error` 可填 `true`、`false` 或子串；`progress`/`logs`/`outputs`（来自 `job_output`）按顺序子串匹配。

脚本扩展主要测加载与日志：

```json
{ "steps": [ { "do": "scripts", "expect": { "loaded": ["main"], "logs": ["ready"] } } ] }
```

## 生成起点：`gs ext init`

```powershell
gs ext init script .\lap-timer --id alice.lap-timer
gs ext init native .\board-tools
gs ext init theme .\night-sky
```

在空文件夹生成含 `AGENTS.md` 与 `tests/basic.json` 的模板（默认 id 为 `my.<文件夹名>`，拒绝 `gamescrcpy.`）：
- `script`：`src/main.ts`（主窗口页面）、`tsconfig.json`、`types/gamescrcpy.d.ts`。
- `native`：CMake 工程、`include/gamescrcpy_ext.h`、`src/native.cpp`（`greet` 节点与 `countdown` 长任务）、`contributes.cli`。编译：`cmake -S . -B .build && cmake --build .build --config Release`。
- `theme`：深色与浅色令牌。

## 命令行子命令

`contributes.cli` 将原生任务暴露为子命令（见[清单参考](manifest.zh-CN.md#cli最多-16-个)）：

```powershell
gs ext .\board-tools --help
gs ext .\board-tools countdown --from 5
gs ext alice.board flash --port COM9 --json
```

首个参数可为文件夹、`.gsext` 或已装扩展 id。参数写 `--名字 值` 或 `--名字=值`（布尔 `true`/`false`、数字、点 `x,y`、区域 `x,y,宽,高`、`json`；缺省取清单 `default`）。输出 `[ 40%] 说明`、缩进日志与结果 JSON（`--json` 输出报告；Ctrl+C 取消；`--hosted` 用 `gs-exthost.exe`）。退出码：`0` 成功，`1` 任务失败，`2` 加载或参数错误。

## 在 anmacgamer 里试

1. 用 `gs pack <文件夹>` 打包（[打包与发布](publishing.zh-CN.md)），在扩展页点**从文件安装…**安装、启用并同意权限。
2. 修改后重打包安装即可覆盖同版本。安装路径：`%APPDATA%\GameScrcpy\extensions\<id>\<版本>\`（便携版 `data\extensions`）。
3. 在程序日志（`%APPDATA%\GameScrcpy\logs`）中查 `[extensions]` 与 `logic …`，在卡片详情查问题与健康状态。
4. 无手机测试投屏窗口：`GameScrcpy.exe --demo-device tests\data\video\emulator_h265.media`。
