# 读屏朗读（`read-aloud`）

简体中文 · [English](README.en.md)

演示会话脚本（`"run": "session"`）配合投屏工具栏按钮（`read`）与侧边面板（`area`）：用 `vision.recognize("system", "ocr", …)` 识别画面区域文字，再用 `speech.say` 朗读。需 `device.screen` 权限。

## 试一试

1. 校验、检查并打包：
   ```zsh
   gs validate examples/extensions/read-aloud
   gs check examples/extensions/read-aloud
   gs pack examples/extensions/read-aloud -o read-aloud.gsext
   ```
2. 在扩展页点**从文件安装…**安装、启用并同意 `device.screen`。
3. 打开投屏窗口（无手机可用 `/Applications/anmacgamer.app/Contents/MacOS/anmacgamer --demo-device tests/data/video/emulator_h265.media`）。
4. 在**读屏朗读**侧边面板调整 Left、Top、Width、Height 与语言（保存在 `store`），点工具栏喇叭按钮或**Read now**即可朗读。

`speech.say(text, { interrupt: true })` 免权限并会打断上一句；朗读期间用 `ui.toolbar("read").set({ checked: true })` 高亮按钮。
