# 事件记录（`event-log`）

简体中文 · [English](README.en.md)

演示用 `events.on` 订阅全部程序与设备事件，并在主窗口页面按最新优先列出（支持暂停、筛选与清空）。接收手机与窗口事件需 `events.app` 权限。

## 试一试

1. 校验、检查并打包：
   ```powershell
   gs validate examples\extensions\event-log
   gs check examples\extensions\event-log
   gs pack examples\extensions\event-log -o event-log.gsext
   ```
2. 在扩展页点**从文件安装…**安装、启用并同意 `events.app`。
3. 打开左栏**事件记录**，连接手机、开始投屏、切换方案或开关扩展即可看到事件列表。

无 `events.app` 时应用脚本仅收到 `extension.enabled`、`extension.disabled` 与 `settings.changed`。各事件载荷见 [sdk/docs/events.md](../../../sdk/docs/events.md)。
