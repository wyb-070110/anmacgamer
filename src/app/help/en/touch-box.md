A touch box is a hardware board added by an extension that acts as your phone's touch screen and keyboard without USB debugging. Once its extension is enabled, the box gets its own card on the **Devices** page.

## Use the ESP32 touch box

The ESP32 touch box comes from a separate community extension, `community.esp32`, made for the Windows version. It has not been tested on macOS, and anmacgamer does not ship it.

1. **Install**: if you have its `community.esp32-<version>.gsext` package, select **Install from file…** on the **Extensions** page, pick the package, and approve its permissions.
2. **Follow its guide**: wiring, firmware flashing, and Wi-Fi setup articles appear in this Help under the extension's name.
3. **Problems on a Mac**: report them in [anmacgamer's issues](https://github.com/wyb-070110/anmacgamer/issues), not to the extension's authors.

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
