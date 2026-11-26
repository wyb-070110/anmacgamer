# 语音命令（`voice-commands`）

简体中文 · [English](README.en.md)

演示 `speech.listen`（听语音命令）、`input.source`（向前台投屏窗口发键）与 `speech.say`（语音回复）。需 `speech.listen` 与 `hardware` 权限。

## 试一试

1. 校验、检查并打包：
   ```zsh
   gs validate examples/extensions/voice-commands
   gs check examples/extensions/voice-commands
   gs pack examples/extensions/voice-commands -o voice-commands.gsext
   ```
2. 在扩展页点**从文件安装…**安装、启用并同意权限。
3. 打开左栏**语音命令**，设置引擎、语言、置信度并管理命令（默认中文“打开地图”“打开背包”“换弹”，英文“open map”“open bag”“reload”，存于 `store`）。
4. 在键位编辑器等按键时说“打开地图”录为 `Ext.example.voice-commands.voice.map`；之后在投屏窗口说出即可按下 80 毫秒并播报回复（`interrupt: true`）。
