Extensions add touch boxes, connection methods, media sources/sinks, speech engines, languages, AI providers, file converters, components, themes, and panels to anmacgamer. No extensions are pre-installed; each only runs what you allow.

See [What extensions add](help:extension-features) to find where each feature appears.

## Install an extension {#install}

1. **Get a package**: download a `.gsext` extension or `.gsbundle` bundle from the author, or click a `gamescrcpy://` install link.
2. **Open it**: on the **Extensions** page, click **Install from file…** or drop the file onto the window.
3. **Review permissions**: check the version, author, signature, and requested permissions, then click **Install** (or **Trust and install** for native code).
4. **Enable it**: toggle the switch on its card on the **Extensions** page.

> [!WARNING]
> Only install extensions from authors you trust. Enabled extensions can use any permissions you approved.

## Install links {#links}

- :import: **`gamescrcpy://` install links**: anmacgamer downloads the package over HTTPS (max 200 MiB), verifies its SHA-256 hash, and opens the install dialog.
- :link: **Extension links**: `gamescrcpy://extension/<id>` opens that extension's card.
- **Portable copies**: register links under **Settings > General > Extension links > Register gamescrcpy:// links**.

> [!NOTE]
> When passing a `gamescrcpy://` link from Terminal, it must be the only argument after `anmacgamer.app/Contents/MacOS/anmacgamer`.

## Signatures & publishers {#signing}

- :shield: **Verified publisher**: valid signature matching the key used when first installed.
- :info: **New signing key**: valid signature, but first time seen on this Mac—verify the key against the author's page. Future updates must use the same key.
- :triangle-alert: **Unsigned**: author and integrity cannot be verified.

Packages with mismatched signatures, unsigned updates to signed extensions, or key changes are blocked (remove the old version first if an author intentionally rotated keys).

## Native code {#native}

Extensions with macOS native libraries (`.dylib`), such as touch box drivers, run in a separate process:

- :shield: **Restricted**: runs sandboxed without access to your files, network, or other apps; reaches devices only through anmacgamer.
- :triangle-alert: **Unrestricted**: runs with your macOS user account's permissions; the install dialog lists detected system capabilities (network, files, input, processes).

Code that injects into other processes, targets the wrong CPU architecture, or fails macOS Gatekeeper/XProtect/signature checks is rejected.

## Manage, export & remove {#manage}

Each card on the **Extensions** page offers:

- :power: **Switch**: enable or disable the extension immediately.
- :sliders-horizontal: **Settings**: configure extension options (also under **Settings > Extensions**, with per-device overrides when a phone is connected).
- :info: **Details**: view added features, permissions, tours ([Guided tours](help:tours#extension-tours)), health logs, and updates.
- :ellipsis: **More**: **Export…** as `.gsext` or **Remove…** from this Mac.

Use **Export bundle…** on the **Extensions** page to pack all enabled extensions and their settings into one `.gsbundle`.

## Updates {#updates}

- **Automatic**: enable **Settings > General > Updates > Check extensions for updates** (checks once a day at startup).
- **Manual**: click **Check for updates** on the **Extensions** page, then click **Update** on any card (or **Skip this version** in **Details**).
- If an update requests new permissions, it stays off until you re-enable it. Signed updates must match the original signing key.

## Compatibility {#compatibility}

Extensions remain compatible as long as the host interfaces they use have not changed. If an extension requires a newer or older anmacgamer version, its card and the startup compatibility dialog explain why and keep it disabled.

## Troubleshooting {#trouble}

- :circle-x: **Auto-disabled**: if an extension's process stops 3 times in a minute (or its modules fail 3 times in 10 seconds), anmacgamer turns it off automatically. Check **Details > Health** for crash reports.
- :search: **Find a problem extension**: click **Find a problem extension** on the **Extensions** page to binary-search which extension causes an issue.
- :shield: **Safe mode**: launch `open -a anmacgamer --args --safe-mode` to start without loading any extensions. (If anmacgamer crashes with extensions loaded, the next launch enters safe mode automatically.)

Attach **Settings > About > Export diagnostics…** when reporting an issue.
