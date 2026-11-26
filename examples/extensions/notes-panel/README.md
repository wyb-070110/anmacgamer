# 笔记（`notes-panel`）

简体中文 · [English](README.en.md)

在主窗口页面演示 `files.folder`（将 `.txt` 笔记存到用户选定目录）与 `net.fetch`（从 `api.github.com` 获取最新版本）。需权限 `files.paths`（`notes`）与 `network`（`api.github.com`）。

## 试一试

1. 校验、检查并打包：
   ```powershell
   gs validate examples\extensions\notes-panel
   gs check examples\extensions\notes-panel
   gs pack examples\extensions\notes-panel -o notes-panel.gsext
   ```
2. 在扩展页点**从文件安装…**安装、启用并同意权限。
3. 打开左栏**笔记**并首次选定目录（可在扩展设置中忘记）。
4. 新增、查看、刷新或删除 `.txt` 笔记，或点**Add the newest GameScrcpy release**从 `api.github.com` 拉取最新发版（其他域名会被 `GS-API-155` 拒绝）。
