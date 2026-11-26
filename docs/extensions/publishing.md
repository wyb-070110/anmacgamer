# Packing and publishing

English · [简体中文](publishing.zh-CN.md)

An extension ships as a `.gsext` zip archive with `manifest.json` at the root. Users install it via **Install from file…** on the Extensions page or a `gamescrcpy://install` link.

## Packing: `gs pack`

```powershell
gs pack .\lap-timer
gs pack .\lap-timer -o .\dist\alice.lap-timer-1.2.0.gsext
gs pack .\lap-timer --sign $env:USERPROFILE\keys\alice.key
```

- Runs `gs validate` first and aborts on errors.
- Default output: `<id>-<version>.gsext` next to the folder (`-o` cannot point inside the source folder).
- Includes all files except dot-prefixed entries (`.git`, `.build`, `.vscode`, etc.).
- Limits: max 4096 files, 64 MiB per file, 256 MiB unpacked total, 100:1 compression ratio.
- `--official` is reserved for built-in `gamescrcpy.*` ids.

## Versions and updates

- Bump `version` (semver) on every release. Keep `id` unchanged so settings, permissions, and signing continuity stay intact.
- Adding permissions, `network` hosts, `files.paths` folders, or required dependencies requires users to re-enable and re-approve the extension.
- Add `updates.index` for in-app update notifications (see [Update lists](#update-lists)), or share an install link.

## Signing

First install of a signed package shows the raw key ("Signed with key …, which this computer has not seen before"); later versions with the same key show "Verified publisher · key …". Unsigned native extensions show a prominent warning (a note only if restricted).

1. Generate a key pair once (remove password from any password-protected minisign key with `minisign -C -W`):
   ```powershell
   gs ext keygen $env:USERPROFILE\keys\alice.key
   ```
2. Add the printed `"authorKey": "RWQ…"` line to `manifest.json`.
3. Pack and sign with `gs pack <folder> --sign <secret-key>`. Mismatched keys fail with `signing-key-mismatch`.

Manual minisign signing is supported: include `SHA512SUMS` (`<128-hex-sha512>  <path>` per file) and `SHA512SUMS.minisig` matching `authorKey`.

Verification rules (also apply to downgrades):

| Condition | Result |
|:--|:--|
| File hash mismatch, added/missing file, or wrong key | Rejected (`signature-invalid`) |
| Installed version is signed, new package is unsigned | Rejected (`signature-missing`) |
| New package signed with a different key | Rejected (`publisher-changed`; user must uninstall first) |

Back up your secret key and publish your public key (for example in your README) so users can verify it on first install.

## Install links

```html
<a href="gamescrcpy://install?url=https%3A%2F%2Fexample.com%2Fdownloads%2Falice.lap-timer-1.2.0.gsext&sha256=9f2b5c0e4d7a8b1c3e5f6a7b8c9d0e1f2a3b4c5d6e7f8091a2b3c4d5e6f70812">Install in GameScrcpy</a>
```

- Parameters: `url` (`https://` with host, no username, URL-encoded, <= 2048 chars decoded) and `sha256` (64 hex digits). Each once, no extra params, no `#`; whole link <= 4096 printable ASCII chars.
- Download limits: <= 8 HTTPS-only redirects, <= 200 MiB. Mismatched SHA-256 aborts install. The dialog shows the final host in ASCII (`xn--…`) after redirects.
- `gamescrcpy://extension/<id>` opens the Extensions page at that card.
- Must be the sole CLI argument (extra args exit with error). Only one link install runs at a time; links arriving while an install dialog is open are dropped with a prompt.
- Portable builds register `gamescrcpy://` under **Settings > General > Extension links**.

Helper commands (PowerShell; regenerate `sha256` each release):

```powershell
(Get-FileHash .\alice.lap-timer-1.2.0.gsext -Algorithm SHA256).Hash.ToLower()
[uri]::EscapeDataString("https://example.com/downloads/alice.lap-timer-1.2.0.gsext")
```

## Update lists

Point `updates.index` ([manifest reference](manifest.md#updates)) to a JSON file serving one or more extensions:

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

| Field | Rule |
|:--|:--|
| `format`, `version` | Must be `gamescrcpy.extension-index` and `1` |
| `extensions` | Max 256 entries, unique `id` |
| `versions` | 1–128 entries per extension, unique `version` |
| `url` | `https://` with host, no username |
| `sha256` | 64 hex digits |
| `size` | 1 byte to 200 MiB |
| `publisher` | Optional minisign public key (same format as `authorKey`) |
| `requires` | Optional, max 32 entries; incompatible versions are skipped |
| `released` | Optional `YYYY-MM-DD` |
| `notes` | Optional, max 4096 bytes |

Rules:
- Max 1 MiB, max 8 nesting levels. Parsed strictly as a whole (any extra/duplicate/invalid field rejects the list and shows the JSON path in Details).
- Offers the highest compatible version newer than both installed and skipped versions; prereleases only offer to installed prereleases.
- Fetched on manual **Check for updates**, or daily on startup if enabled in Settings > General (off by default). HTTPS only, <= 8 redirects, system proxy, no cookies/cache, 30 s timeout, extension `id` is not sent.
- Downloads only on user click (one at a time, cancellable), verifying `size`, `sha256`, `id`, `version`, compatibility, `publisher`, and existing signature continuity, then opens the install dialog showing permission/dependency diffs. Nothing installs automatically.
- Upload the new `.gsext` before updating the index JSON.

## Bundles

**Export bundle…** packs enabled extensions and settings into a `.gsbundle` zip containing `bundle.json` (`schemaVersion` 1, `name`, `description`, `extensions` list) and `extensions/<id>-<version>.gsext` (max 64 extensions). Validate with `gs validate <bundle>`.

## Before you publish

- `gs validate` (0 errors), `gs check`, and `gs ext test` (plus `--hosted` for native) all pass.
- `requires` declares exact extension point ranges (`"native": "^2.0"` for native) and `"gamescrcpy": "2.1"` is set (otherwise `gamescrcpy-missing` warning).
- `id` uses your own prefix; `license`, `authors`, `repository`, `description`, permission `reason`s, and `l10n`/`locales` translations are filled in.
- Native DLLs link runtime statically (`/MT`), export `gs_ext_main`, include arm64 if possible, and use `"sandbox": "restricted"` when feasible.
- Package is signed with `gs pack --sign`, secret key backed up, public key published, `updates.index` updated after upload, and tested by installing from file in anmacgamer.
