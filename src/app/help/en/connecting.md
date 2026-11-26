anmacgamer mirrors your Android phone on your Mac so you can control it with a keyboard and mouse. Connect once with a USB cable; after that, you can also connect over Wi-Fi.

## Prepare the phone {Only once}

1. **Pick your phone brand**: in the setup guide, select your phone brand to see where Developer options and USB debugging live.
2. **Turn on USB debugging**: follow the steps shown for your brand.

![Prepare the phone](figure:setup-steps)

> [!TIP]
> Cannot find Developer options? Search your phone's Settings for "developer", or [open the guide](action:guide).

## Connect with a USB cable

1. **Plug in a data cable**: connect the phone to your Mac using a USB cable that carries data, not a charge-only cable.
2. **Choose File transfer**: when the phone asks what the USB connection is for, tap **File transfer**.
3. **Allow USB debugging**: unlock the phone, check **Always allow from this computer**, and tap **Allow**.
4. **Start mirroring**: select **Mirror** on the phone's card on the **Devices** page. Closing the mirror window ends the session.

![Connect with a USB cable](figure:phone-usb,phone-usbmode,phone-allow!,phone-ready)

![The device card shows connection progress until the mirror window opens.](figure:device-stepper)

Phone not showing up? See [FAQ: The phone does not show up](help:faq#no-phone).

## Pair over Wi-Fi {Android 11 or later}

Keep your Mac and phone on the same Wi-Fi network. On the **Devices** page, select **Wireless pairing** to open the **Wireless debugging** section.

### With a pairing code {icon=type}

1. **Open the pairing dialog**: on the phone, go to **Developer options > Wireless debugging > Pair device with pairing code**.
2. **Select your phone**: anmacgamer detects the dialog automatically. If it does not appear, select **Type the address** and enter the IP address and port shown on the phone.
3. **Enter the 6-digit code**: pairing starts as soon as all six digits are entered.

![Pair with a pairing code](figure:phone-pairing,,!)

### With a QR code {icon=qr-code}

1. **Show the QR code**: select **Use a QR code** in anmacgamer.
2. **Scan it**: on the phone, go to **Developer options > Wireless debugging > Pair device with QR code** and scan the screen.

![Pair with a QR code](figure:pair-qr,phone-scan!)

### After pairing {#after-pairing}

The phone usually connects automatically. If it does not, enter the main **Wireless debugging** IP address and port (not the pairing port) under **Recent connections** and select **Connect**. Paired phones stay in **Recent connections** for one-click reconnection later.

> [!WARNING]
> Android 10 and earlier cannot pair over wireless debugging. Connect by USB first and use [Connect over the network](help:connecting#network).

## Connect over the network {#network}

Use port 5555 network debugging for Android 10 and earlier, or when connecting over Ethernet without Wi-Fi.

1. **Connect by USB**: plug the phone into your Mac.
2. **Enable network debugging**: open **More (…)** on the phone's card and select **Connect over the network**.
3. **Switch to the network**: unplug the USB cable and connect the phone to the same network as your Mac. It will appear under **Recent connections**.

Network debugging resets when the phone restarts; repeat these steps over USB if needed.

## Connection modes & device settings

Before clicking **Mirror**, you can pick how video travels on the phone's card:

- :plug: **adb tunnel**: works over USB, wireless debugging, or network debugging in any network setup.
- :wifi: **WiFi direct**: sends video directly over the local network for lower latency (requires wireless/network debugging and no router AP isolation).
- :sparkles: **Auto**: uses WiFi direct for wireless phones and adb tunnel for USB phones.

You can mirror multiple phones at once—each gets its own window. Click the settings icon on any phone's card to override its resolution, frame rate, codec, bitrate, or audio independently. Use **More (…)** on the card for **Apps**, screen/orientation controls, navigation keys, diagnostics, **Connect over the network**, or **End wireless debugging connection**.
