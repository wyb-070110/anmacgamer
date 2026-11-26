Quick fixes for common issues.

## Connecting {icon=smartphone} {tone=azure}

### The phone does not show up on the Devices page {#no-phone}

1. Use a data-capable USB cable (not charge-only).
2. Turn on **USB debugging** in Developer options.
3. Set the phone's USB mode to **File transfer**.
4. Unlock the phone and tap **Allow** on the "Allow USB debugging?" prompt.

> [!TIP]
> Need brand-specific steps? [Open the guide](action:guide).

### The phone says "Waiting for authorization" {#waiting-authorization}

Unlock the phone, tick **Always allow from this computer**, and tap **Allow**.

### The phone is "Offline" {#offline}

Unplug and replug the USB cable. If it stays offline, toggle **USB debugging** off and back on.

### The Devices page says adb is not available {#adb-missing}

Install Android SDK Platform-Tools (e.g. `brew install --cask android-platform-tools`), make sure no other app is blocking adb, then click **Retry** on **Devices**.

### USB debugging is greyed out on a Samsung phone {#samsung-grey}

Turn off **Settings > Security and privacy > Auto Blocker** on the phone first.

## Picture and sound {icon=monitor-play} {tone=orange}

### There is no sound {#no-sound}

- Sound requires Android 11 or later (unlock Android 11 phones before connecting).
- Check **Settings > Audio > Sound** is on and the mirror toolbar is not muted.
- Note: while mirroring, volume is controlled from your Mac; the phone's volume buttons do not change it.

### WiFi direct does not connect {#wifi-direct}

Keep Mac and phone on the same network, allow **anmacgamer** in **System Settings > Privacy & Security > Local Network**, and ensure router AP isolation and UDP firewalls are not blocking it. Otherwise, switch to **adb tunnel** on the phone's card.

### The picture stutters or lags {#lag}

- Connect over USB for the steadiest frame rate.
- Lower **Maximum resolution** or **Bit rate limit** in **Settings > Mirroring & quality**, or enable **Low-latency mode**.
- Check the diagnostics overlay for FPS/latency and confirm Metal hardware acceleration is active (VMs without Metal fall back to CPU rendering).

### Chinese and other characters do not reach the phone {#typing}

Non-ASCII characters use the phone's clipboard unless you type through [anmacgamer Companion](help:companion). Check if the phone blocks background clipboard access, or enable Companion's input method.

## Keys and control {icon=keyboard} {tone=lime}

### The picture works, but the mouse and keyboard do nothing {#no-input}

Enable the extra developer switch for your brand, then reconnect:

| Brand | Switch to turn on |
|---|---|
| Xiaomi · Redmi · POCO | **USB debugging (Security settings)** (requires a signed-in Mi account; reboot after enabling). |
| OPPO · OnePlus · realme | **Disable permission monitoring** (on ColorOS 16+, switch phone language to English temporarily to see **Disable system optimization**). |
| vivo · iQOO | **USB simulate click**. |

### The keys of my profile do nothing {#keys-nothing}

- Make sure **Key mapping** is toggled on in the mirror toolbar.
- Pick a profile in **Key mapping options**.
- Click inside the mirror window so it is focused.

### The mouse pointer is gone {#pointer-gone}

Camera lock is active. Press the camera lock key again (the key above Tab, left of 1, or <kbd>Tab</kbd> in built-in Honkai: Star Rail profiles).

### A keyboard shortcut does nothing {#shortcut-nothing}

Check **Settings > Keyboard shortcuts** for conflicts. Active profile keys take priority over app shortcuts in the mirror window.

## Other {icon=settings-2} {tone=gray}

### anmacgamer closed unexpectedly {#crashed}

Click **Export diagnostics…** on the next launch (or in **Settings > About**) to save a zip with logs and crash reports (personal data like serials, IPs, and clipboard text are stripped).

### An extension causes trouble {#extension-trouble}

Use **Find a problem extension** on **Extensions**, or launch `open -a anmacgamer --args --safe-mode` to disable all extensions for one session. See [Extensions · Troubleshooting](help:extensions#trouble).

### Where are my settings and profiles? {#data-folder}

In `~/Library/Application Support/anmacgamer` (`profiles/` and `logs/`). Set `GS_DATA_DIR` before launch to use a custom folder.

### Which phones work? {#which-phones}

Android 9 or later. HarmonyOS 5 and later is not Android and cannot be mirrored.

### Where can I find all commands? {#all-commands}

Press <kbd>⌘ Command</kbd>+<kbd>K</kbd> for the command palette, or <kbd>F1</kbd> for Help.
