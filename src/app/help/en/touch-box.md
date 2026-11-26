A touch box is a hardware board added by an extension that acts as your phone's touch screen and keyboard without USB debugging. Once its extension is enabled, the box gets its own card on the **Devices** page.

## Use the ESP32 touch box

The ESP32 touch box is provided by the separate community extension `community.esp32`.

1. **Download**: get `community.esp32-<version>.gsext` from [GameScrcpy-ESP32](https://github.com/NuoRan/GameScrcpy-ESP32).
2. **Install**: on the **Extensions** page, select **Install from file…**, pick the package, and approve its permissions.
3. **Follow its guide**: wiring, firmware flashing, and Wi-Fi setup articles appear in this Help under the extension's name.

> [!NOTE]
> Settings from GameScrcpy 2.0 do not carry over. Re-select your touch method and set up the board's Wi-Fi in the new extension.

## Manage a box on the Devices page

- :mouse-pointer-click: **Control**: opens a no-picture control window that sends keys and mouse input through the box.
- :sliders-horizontal: **Settings**: opens the box's own settings window (if provided by the extension).
- :ellipsis: **More**: extra actions from the extension, such as flashing firmware.

If the status says "In use by …", another session or extension has the serial port open; close that first.

## Pair a box with a phone or capture card

- **With a mirrored phone**: open the phone's **Settings > Input & key mapping**, choose the box under **Touch method**, and select the hardware under **Device for the touch method**. It takes effect on the next connection.
- **With a capture card**: open **Settings** on the capture card's card and pick the box under **Touch > Touch method**. See [Capture cards](help:capture-card).

> [!TIP]
> The box only appears in touch method lists while its extension is enabled and the board is connected.
