A capture card brings your phone's HDMI video into anmacgamer with key mapping, vision, and recording—even when USB debugging is off. Touches are sent over AOA or a touch box.

> [!NOTE]
> Capture card video input is not available on macOS yet (use USB debugging, wireless debugging, or Companion instead). When supported, only standard UVC capture cards recognized natively by macOS work.

## Hardware needed

- :usb: **USB 3.0 capture card**: delivers 1080p60 with minimal delay.
- :plug: **USB 2.0 capture card**: uses MJPEG compression (slightly softer image, GPU-decoded).
- :smartphone: **USB-C to HDMI adapter**: your phone must support video output over USB-C (DisplayPort Alt Mode).

## Connect a capture card

1. **Wire it up**: connect Phone -> USB-C to HDMI adapter -> HDMI cable -> Capture card -> Mac USB 3.0 port.
2. **Wake the phone**: turn on the phone screen so it outputs video.
3. **Start mirroring**: select **Mirror** on the capture card's card on the **Devices** page.

If a webcam appears as a capture card, choose **More > This is not a capture card, hide it** to move it to **Other video devices** at the bottom. You can restore any hidden device with **Treat as capture card**.

## Configure touch and profile

Open **Settings** on the capture card's card:

- **Touch > Touch method**: choose **No touch** (view/record only), **AOA over USB** (also plug the phone into USB and pick it under **Phone on USB**; see [AOA touch over USB](help:aoa-driver)), or a connected **Touch box** (see [Touch boxes](help:touch-box)).
- **Key mapping > Profile**: pick the profile manually, since anmacgamer cannot detect the foreground app without adb.
- **Picture and sound > Sound**: toggle audio capture and recording on or off.

## Crop black bars & set rotation

- **Auto crop**: anmacgamer automatically trims black bars around the phone screen. To adjust manually, choose **More > Adjust the picture area…** in the mirror window, drag the box, and click **Apply** (or **Back to automatic** to reset).
- **Orientation**: without adb, choose **Landscape (turned left)** or **Landscape (turned right)** under **Which way the phone turns** on the toolbar so touches do not land upside down.
