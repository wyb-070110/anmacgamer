# 按键桥（`deck-bridge`）

简体中文 · [English](README.en.md)

演示 `net.listen`（`127.0.0.1` 本机 HTTP 服务）、`input.source`（通过输入源 `deck` 向键位发送按键）与 `ext.provide`（跨扩展服务）。需权限 `network.listen` 与 `hardware`。

## 试一试

1. 校验、检查并打包：
   ```powershell
   gs validate examples\extensions\deck-bridge
   gs check examples\extensions\deck-bridge
   gs pack examples\extensions\deck-bridge -o deck-bridge.gsext
   ```
2. 在扩展页点**从文件安装…**安装、启用并同意权限。
3. 在左栏打开**按键桥**（`Listening on http://127.0.0.1:8765`）。
4. 在键位编辑器等待录键时运行：
   ```powershell
   Invoke-RestMethod -Method Post http://127.0.0.1:8765/tap/k1
   ```
   即可录为 `Ext.example.deck-bridge.deck.k1`。从 Stream Deck 或 OBS 发同一 `POST` 请求即可触发（发往前台投屏窗口）。

## 接口与服务

| 请求 | 作用 |
|:--|:--|
| `GET /` | 返回 `{ listening, port, held }` |
| `POST /tap/<键>?ms=80` | 点按 `<键>`，`ms` 毫秒后松开（10–2000，默认 80） |
| `POST /press/<键>` / `POST /release/<键>` | 按住或松开 `<键>`（`[a-zA-Z0-9_-]{1,32}`） |

按键操作仅接受 `POST`。依赖 `example.deck-bridge` 的其他扩展可调用 `ext.call("example.deck-bridge/status")` 或 `ext.call("example.deck-bridge/tap", { key: "k1", ms: 120 })`（`tap` 需 `hardware`）。在详情关闭本机连接会停服，重开后需点**Listen**。
