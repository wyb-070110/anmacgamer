# Tour and help (`tour-and-help`)

[简体中文](README.md) · English

Demonstrates `contributes.tours` (5-step tour) and `contributes.help` (two bilingual articles under `help/`), paired with a sidebar page (`where: "page"`) and Settings card (`where: "settings"`).

## Try it

1. Validate, check, and pack:
   ```powershell
   gs validate examples\extensions\tour-and-help
   gs check examples\extensions\tour-and-help
   gs pack examples\extensions\tour-and-help -o tour-and-help.gsext
   ```
2. Install via **Install from file…** and enable it.
3. On first enable, the tour starts automatically and highlights the extension card, sidebar page, and Settings card (render panels on startup so targets are not skipped).
4. Press `F1` to view both articles under **Extensions**. Replay the tour from extension Details or via `action:tour:ext/example.tour-and-help/intro` (`help:ext/example.tour-and-help/writing` links between articles).

Tour strings use `l10n` (`tours.intro.title`, `tours.intro.<step>.title`, `tours.intro.<step>.text`); article bodies use `locales`.
