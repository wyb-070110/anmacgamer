**anmacgamer Companion** is a phone app that shows your Mac's cursor and key hints on the phone screen, lets you type with your Mac's keyboard, and streams picture and sound over Wi-Fi when USB debugging is off. Mirroring and key mapping over USB debugging work without it.

## What it does {#features}

- :mouse-pointer-click: **Cursor on the phone**: shows where your Mac pointer is over the phone screen.
- :keyboard: **Key hints**: displays the active profile's key hints on the phone screen.
- :type: **Mac keyboard input**: types directly into phone text boxes (including Chinese) using the Companion input method.
- :wifi: **Wi-Fi mirroring & auto-connect**: pairs over QR code to stream picture and sound without USB debugging, and reconnects automatically on the same Wi-Fi.

## Install the companion {#install}

- **With USB debugging**: when you turn on a Companion feature in anmacgamer, click **Install** in the prompt and confirm on the phone. Newer versions update automatically.
- **Without USB debugging**: scan the QR code shown in the control window with your phone's camera to download and install the APK. See [AOA touch over USB](help:aoa-driver).

## Manage switches on either side {#switches}

The Companion app is a single Settings-style page with **status**, **Features**, **Computers**, **Permissions**, and **About** sections. Changes sync between Mac and phone automatically when connected.

- **On Mac**: open **Settings > Input & key mapping > anmacgamer Companion on the phone** to toggle keyboard input, phone key hints, cursor display, and Wi-Fi auto-connect (globally or per phone).
- **On phone**: toggle switches directly under **Features**. Changes made offline sync on the next connection.

> [!NOTE]
> Cursor size/style and the typing hint bar position (Bottom/Top) are stored on the phone only.

## Key hint opacity {#opacity}

Key hints on the phone appear at full brightness briefly, then fade to your chosen opacity (**Settings > Input & key mapping > anmacgamer Companion on the phone > Opacity**, or **Features > Key hints > Opacity once faded** on the phone).

## Pair and reconnect over Wi-Fi {#wifi}

1. **Same Wi-Fi**: connect your Mac and phone to the same Wi-Fi network.
2. **Scan once**: tap **Scan to connect** in Companion and scan the QR code in anmacgamer's control window (one scan per computer).
3. **Reconnect**: with **Auto connect** on, Companion reconnects automatically. Manage saved Macs under **Computers** (**Connect now** or **Remove**).

If it cannot connect, check that **anmacgamer** is allowed under **System Settings > Privacy & Security > Local Network** and in the macOS Firewall.

## Permissions {#permissions}

Open **Permissions** in Companion and tap **Open** on any missing item:

| Permission | Purpose |
|---|---|
| Display over other apps | Show cursor and key hints over games. |
| Notifications | Show connection status with a quick Stop button. |
| Camera | Scan the pairing QR code. |
| Input method | Type with the Mac keyboard (auto-enabled with USB debugging; enable and tap **Choose** manually without it). |
| Screen capture | Allow screen capture when requested without USB debugging. |

> [!TIP]
> Companion communicates only with anmacgamer on your own computers over USB or local Wi-Fi and collects no data.
