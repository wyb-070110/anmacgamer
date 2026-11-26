AOA lets your Mac act as a USB touch screen, mouse, and keyboard for your phone without turning on USB debugging. macOS needs no extra driver.

## Turn on AOA

1. **Plug in a data USB cable**: connect your phone to the Mac and unlock it. With USB debugging off, the phone shows up on the **Devices** page as **USB · AOA**.
2. **Select Mirror**: the control window opens, and your keyboard and mouse control the phone over the cable.

With USB debugging on, touches can go over AOA too: go to **Settings > Input & key mapping > Touch** and choose **AOA** (globally or for one phone). It applies the next time the phone connects.

## Stream picture and sound with Companion

Without USB debugging, AOA handles touch over the USB cable while **anmacgamer Companion** sends the phone's picture and sound over Wi-Fi.

1. **Connect to the same Wi-Fi**: keep your Mac and phone on the same Wi-Fi network.
2. **Install the companion**: if not installed yet, scan the QR code in the control window with your phone's camera to download and install it.
3. **Pair by QR code**: open **anmacgamer Companion**, tap **Scan to connect**, allow camera access, and scan the QR code in the control window. You only need to scan once per computer.
4. **Allow macOS Local Network**: when prompted, allow anmacgamer in **System Settings > Privacy & Security > Local Network** (and in the macOS Firewall if enabled).

While mirroring over Companion, the phone stays silent and sound plays on your Mac. Next time, Companion reconnects automatically when the control window opens.

## Control window (without picture)

If Companion is not connected and debugging is off, a control window opens showing key hints on a dark canvas ("No picture · Watch the phone").

- **Input**: keyboard and mouse control the phone only while the control window is active.
- **Pointer**: with a free cursor, the phone shows its own pointer; locking the camera hides it.
- **Orientation**: without adb, anmacgamer cannot detect rotation. Set **Portrait**, **Landscape (turned left)**, or **Landscape (turned right)** from the toolbar's orientation menu.

## Enable Mac keyboard typing

1. Open **anmacgamer Companion** on your phone and go to **Permissions**.
2. Tap **Open** on **Input method**, enable **anmacgamer Companion**, then return and tap **Choose** to select it.
3. While an input box is open on the phone, a thin bar lets you switch back to the phone's keyboard.

See [anmacgamer Companion](help:companion) for more details.

## What still requires a picture

If you run without Companion video or debugging, features that read the screen are unavailable: vision workbench, `vision.*` script calls, recording, clipboard sharing, app launching, real latency measurement, turning off the phone screen, and auto-rotation. Key mapping, camera lock, and touch logic still work.
