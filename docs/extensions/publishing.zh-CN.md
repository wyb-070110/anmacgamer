# 打包与发布

[English](publishing.md) · 简体中文

扩展以 `.gsext`（zip）发布，根目录放 `manifest.json`。用户在扩展页点**从文件安装…**或点击 `gamescrcpy://install` 链接安装。

## 打包：`gs pack`

```powershell
gs pack .\lap-timer
gs pack .\lap-timer -o .\dist\alice.lap-timer-1.2.0.gsext
gs pack .\lap-timer --sign $env:USERPROFILE\keys\alice.key
```

- 先执行 `gs validate`，有错误则不打包。
- 默认输出为文件夹旁的 `<id>-<version>.gsext`（`-o` 不能指向源文件夹内部）。
- 打包文件夹内除点开头项（`.git`、`.build`、`.vscode` 等）外的全部文件。
- 上限：最多 4096 个文件、单文件 64 MiB、解压总计 256 MiB、压缩比 100:1。
- `--official` 仅供官方 `gamescrcpy.*` 扩展使用。

## 版本号与更新

- 每次发布按语义化版本递增 `version`。不要修改 `id`，以保留用户设置、权限与签名延续性。
- 新增权限、`network` 域名、`files.paths` 文件夹或必需依赖时，用户需重新启用并同意。
- 填写 `updates.index` 可支持应用内更新提示（见[更新列表](#更新列表)），或直接提供安装链接。

## 签名

首次安装签名包时显示原始公钥（“已用公钥 … 签名，这台电脑以前没见过这个公钥”）；后续同公钥版本显示“已验证的发布者 · 公钥 …”。未签名原生扩展会显示醒目警告（受限模式仅提示）。

1. 生成密钥对一次（带密码的 minisign 私钥可用 `minisign -C -W` 去除密码）：
   ```powershell
   gs ext keygen $env:USERPROFILE\keys\alice.key
   ```
2. 将输出的 `"authorKey": "RWQ…"` 写入 `manifest.json`。
3. 打包签名：`gs pack <文件夹> --sign <私钥>`。密钥不匹配报 `signing-key-mismatch`。

也支持手动 minisign 签名：包内提供 `SHA512SUMS`（每行 `<128位十六进制SHA-512>  <包内路径>`）与匹配 `authorKey` 的 `SHA512SUMS.minisig`。

校验规则（同样适用于降级回退）：

| 情况 | 结果 |
|:--|:--|
| 文件哈希不符、增删文件或公钥不匹配 | 拒绝安装（`signature-invalid`） |
| 已装版本有签名，新包无签名 | 拒绝安装（`signature-missing`） |
| 新包换了其他公钥签名 | 拒绝安装（`publisher-changed`，需先卸载再装） |

请备份私钥并在仓库或官网公布公钥，方便用户首次核对。

## 安装链接

```html
<a href="gamescrcpy://install?url=https%3A%2F%2Fexample.com%2Fdownloads%2Falice.lap-timer-1.2.0.gsext&sha256=9f2b5c0e4d7a8b1c3e5f6a7b8c9d0e1f2a3b4c5d6e7f8091a2b3c4d5e6f70812">在 GameScrcpy 里安装</a>
```

- 参数：`url`（带主机名、无用户名的 `https://`，URL 编码，解码后 <= 2048 字符）与 `sha256`（64 位十六进制）。各出现一次、无其他参数、无 `#`；整条链接 <= 4096 可打印 ASCII 字符。
- 下载限制：最多 8 次 HTTPS 重定向、最大 200 MiB。SHA-256 不符即中止。安装框显示重定向后的最终主机（国际化域名以 `xn--…` 显示）。
- `gamescrcpy://extension/<id>` 直接打开扩展页并定位到该卡片。
- 链接必须是唯一命令行参数（带额外参数会报错退出）。同时仅处理一个链接；安装框打开时新链接会被忽略并提示。
- 便携版可在**设置 > 通用 > 扩展链接**一键注册协议。

PowerShell 计算示例（每次发版需更新 `sha256`）：

```powershell
(Get-FileHash .\alice.lap-timer-1.2.0.gsext -Algorithm SHA256).Hash.ToLower()
[uri]::EscapeDataString("https://example.com/downloads/alice.lap-timer-1.2.0.gsext")
```

## 更新列表

将 `updates.index`（见[清单参考](manifest.zh-CN.md#updates)）指向 JSON 文件，一份列表可覆盖多个扩展：

```json
{
  "format": "gamescrcpy.extension-index",
  "version": 1,
  "extensions": [
    {
      "id": "alice.lap-timer",
      "versions": [
        {
          "version": "1.3.0",
          "url": "https://example.com/downloads/alice.lap-timer-1.3.0.gsext",
          "sha256": "9f2b5c0e4d7a8b1c3e5f6a7b8c9d0e1f2a3b4c5d6e7f8091a2b3c4d5e6f70812",
          "size": 48213,
          "publisher": "RWQ…",
          "requires": { "script": "^1.1", "panel": "^1.0" },
          "released": "2026-12-01",
          "notes": "Laps can be named."
        }
      ]
    }
  ]
}
```

| 字段 | 规则 |
|:--|:--|
| `format`、`version` | 固定为 `gamescrcpy.extension-index` 与 `1` |
| `extensions` | 最多 256 个扩展，`id` 唯一 |
| `versions` | 每个扩展 1–128 个版本，`version` 唯一 |
| `url` | 带主机名、无用户名的 `https://` |
| `sha256` | 64 位十六进制 |
| `size` | 1 字节至 200 MiB |
| `publisher` | 可选，minisign 公钥（格式同 `authorKey`） |
| `requires` | 可选，最多 32 项；不兼容版本自动跳过 |
| `released` | 可选，`YYYY-MM-DD` |
| `notes` | 可选，最多 4096 字节 |

规则：
- 最大 1 MiB、嵌套不超过 8 层。整份严格解析（多余、重复或错误字段都会使整份失效并在详情标出 JSON 路径）。
- 提示比已装版本与用户跳过版本都新的最高兼容版；预发布版仅提示给已装预发布版的用户。
- 触发时机：手动点**检查更新**，或设置 > 通用开启后每日启动最多查一次（默认关）。仅 HTTPS、最多 8 次重定向、走系统代理、无 Cookie/缓存、30 秒超时，不发送扩展 `id`。
- 仅在用户点击更新后下载（单任务可取消），核对 `size`、`sha256`、`id`、`version`、兼容性、`publisher` 与签名连续性，再打开安装框展示权限与依赖变化，从不自动安装。
- 发布顺序：先上传 `.gsext`，再更新索引 JSON。

## 整合包

**导出整合包…**将已启用扩展及设置打包为 `.gsbundle`（zip），内含 `bundle.json`（`schemaVersion` 1、`name`、`description`、`extensions`）与 `extensions/<id>-<version>.gsext`（最多 64 个）。可用 `gs validate <整合包>` 校验。

## 发布前检查

- `gs validate`（0 错误）、`gs check`、`gs ext test`（原生再跑 `--hosted`）全部通过。
- `requires` 写明扩展点版本（原生为 `"native": "^2.0"`），并填写 `"gamescrcpy": "2.1"`（缺省报 `gamescrcpy-missing` 警告）。
- `id` 使用自有前缀，补全 `license`、`authors`、`repository`、`description`、权限 `reason` 与 `l10n`/`locales` 译文。
- 原生 DLL 用 `/MT` 静态链接运行库、导出 `gs_ext_main`、尽量提供 arm64，可行时使用 `"sandbox": "restricted"`。
- 用 `gs pack --sign` 签名、备份私钥、公布公钥、先传包再改 `updates.index`，并在 anmacgamer 中从文件实测安装。
