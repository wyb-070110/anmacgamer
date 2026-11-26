# 引导与帮助示例（`tour-and-help`）

简体中文 · [English](README.en.md)

演示 `contributes.tours`（5 步引导）与 `contributes.help`（`help/` 下两篇中英文帮助），并配套一个主窗口页面（`where: "page"`）和设置卡片（`where: "settings"`）。

## 试一试

1. 校验、检查并打包：
   ```zsh
   gs validate examples/extensions/tour-and-help
   gs check examples/extensions/tour-and-help
   gs pack examples/extensions/tour-and-help -o tour-and-help.gsext
   ```
2. 在扩展页点**从文件安装…**安装并启用。
3. 首次启用后自动播放引导，依次高亮扩展卡片、左栏页面与设置卡片（脚本启动时即 `render` 面板，避免跳过步骤）。
4. 按 `F1` 在**扩展**分组查看两篇文章；可在扩展详情重放引导，或用 `action:tour:ext/example.tour-and-help/intro`（文章间互链用 `help:ext/example.tour-and-help/writing`）。

引导文案走 `l10n`（`tours.intro.title`、`tours.intro.<步>.title`、`tours.intro.<步>.text`）；帮助正文走 `locales`。
