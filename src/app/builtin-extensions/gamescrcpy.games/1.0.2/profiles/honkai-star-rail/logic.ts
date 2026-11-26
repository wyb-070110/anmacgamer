// 崩坏：星穹铁道（PC 键位）自动识别跑图 / 战斗模式
// - 自动识别：每 600ms 用 macOS 系统 OCR 检测界面特征词，自动切换 vars.inCombat
//   - 进战特征：左上角波次标（如 1/1、1/2）或右下角技能标签（单攻/扩散/群攻/弹射/辅助/回复/防御/妨害/终结技/释放）
//   - 脱战特征：右侧「状态效果」连续出现 2 次后切回跑图（防止大招动画期间误切）
// - 手动兜底：按 ` (Backquote) 立即切换模式（5 秒内不被自动识别覆盖）
// - 开关自动识别：按 F8 暂停或恢复自动识别（Mac 键盘要按住 fn）
// - 鼠标：默认是普通光标；跑图时按 Tab 锁定视角（鼠标转视角，左键普攻，右键冲刺），再按 Tab 放开，
//   锁定时按住 Alt 临时显示光标；进入战斗自动放开

/** @category 模式识别 */
let autoDetect: boolean = true;
let manualHoldUntil: number = 0;
let overworldHits: number = 0;

// 左上角波次区域（如 1/1、1/2、2/3）
const WAVE_REGION = { x: 0.03, y: 0.0, width: 0.09, height: 0.06 };
// 右下角战斗技能标签区域（单攻 / 扩散 / 群攻 / 弹射 / 辅助 / 回复 / 防御 / 妨害）
const COMBAT_SKILL_REGION = { x: 0.74, y: 0.72, width: 0.26, height: 0.26 };
// 右侧「状态效果」按钮区域（跑图界面特有）
const OVERWORLD_REGION = { x: 0.84, y: 0.14, width: 0.16, height: 0.12 };

const WAVE_PATTERN = /\b\d+\s*\/\s*\d+\b/;
const COMBAT_KEYWORDS = ["单攻", "扩散", "群攻", "弹射", "辅助", "回复", "防御", "妨害", "终结技", "释放"];
const OVERWORLD_KEYWORDS = ["状态效果"];

// 进入战斗时放开光标（跑图时由 Tab 锁定，不自动锁定）
function followMode(combat: boolean): void {
    if (combat) {
        controls.view.unlock();
    }
}

function setMode(combat: boolean, reason: string): void {
    if (vars.inCombat.value === combat) {
        return;
    }
    vars.inCombat.value = combat;
    followMode(combat);
    ui.toast(
        combat
            ? `${reason}：战斗模式（Q普攻 / E战技 / 1-4大招 / V自动 / B二倍速）`
            : `${reason}：跑图模式（WASD移动 / E秘技 / F交互 / Alt显光标）`
    );
}

// 手动切换跑图 / 战斗模式
keys["Backquote"].onPress(() => {
    manualHoldUntil = time.now() + 5000;
    overworldHits = 0;
    setMode(!vars.inCombat.value, "手动切换");
});

// F8 开关自动识别
keys["F8"].onPress(() => {
    autoDetect = !autoDetect;
    ui.toast(autoDetect ? "已开启战斗自动识别" : "已暂停战斗自动识别（按 ` 手动切换）");
});

// 定时 OCR 识别当前处于战斗还是跑图
every(600, async () => {
    if (!autoDetect || time.now() < manualHoldUntil) {
        return;
    }

    if (!vars.inCombat.value) {
        // 先查右下角技能标签（单攻/辅助/扩散等）
        const skillText = await vision.readText({
            region: COMBAT_SKILL_REGION,
            language: "zh-Hans",
            timeoutMs: 400,
        });
        if (skillText && COMBAT_KEYWORDS.some((word) => skillText.text.includes(word))) {
            overworldHits = 0;
            setMode(true, "自动识别");
            return;
        }

        // 再查左上角波次标（1/1、1/2 等，敌方回合也常驻）
        const waveText = await vision.readText({
            region: WAVE_REGION,
            language: "en",
            timeoutMs: 400,
        });
        if (waveText && WAVE_PATTERN.test(waveText.text)) {
            overworldHits = 0;
            setMode(true, "自动识别");
        }
        return;
    }

    // 当前在战斗：连续 2 次在右侧检测到「状态效果」再切回跑图
    const overworldText = await vision.readText({
        region: OVERWORLD_REGION,
        language: "zh-Hans",
        timeoutMs: 400,
    });
    if (overworldText && OVERWORLD_KEYWORDS.some((word) => overworldText.text.includes(word))) {
        overworldHits += 1;
        if (overworldHits >= 2) {
            overworldHits = 0;
            setMode(false, "自动识别");
        }
    } else {
        overworldHits = 0;
    }
});
