# macOS Port Notes

anmacgamer ports [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy) (Apache-2.0) to macOS 14 Sonoma or later (Apple silicon and Intel, universal). Platform-specific code stays in dedicated folders so core modules remain cross-platform.

## Status

All five port stages are done:

- Core modules (`base`, `protocol`, `input`, `vision`, `tsstrip`, `blueprint`) and POSIX platform layer (`adb`, `session`).
- Video and audio: VideoToolbox H.264/H.265, dav1d AV1, Qt RHI (Metal) display-paced rendering, SDL3 audio.
- UI and hardware: Liquid Glass UI, native menu bar and notifications, key/mouse mapping without Input Monitoring permission, Xbox/PS controllers with rumble, four-mode trackpad swipes, libusb USB/AOA touch, IOKit `/dev/cu.*` serial ports.
- Automation and packaging: `sandbox_init`-confined `gs-scripthost` and `gs-exthost` (`.dylib` extensions), Vision OCR, AVFoundation/Speech TTS and voice commands, MP4 recording, universal `.dmg`.

Not yet implemented: HDMI capture cards, Bluetooth LE peripherals, custom extension video shader filters (HLSL on Metal), in-editor TypeScript type checking on Intel Macs (scripts still run), and Apple Developer ID notarization.

CI runs `.github/workflows/macos.yml` on `macos-15` (Apple silicon) for Debug and Release unit tests; device tests are skipped in CI.

## Build and test

Install tools (`ffmpeg`, `sdl3`, `libusb` and Qt 6.10.3 at `~/Qt/6.10.3/macos` are needed for the full app):

```bash
xcode-select --install
brew install cmake ninja openssl@3 android-platform-tools
brew install ffmpeg sdl3 libusb
```

Run core workflows or the full app build:

```bash
cmake --workflow --preset posix-core
cmake --workflow --preset posix-core-release
cmake --preset mac-app && cmake --build --preset mac-app && ctest --preset mac-app
```

Pass `-DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)` if OpenSSL is in a non-default prefix. On the first run of `gs_platform_tests`, macOS creates `GameScrcpy / user-protection-key` in Keychain; choose **Always Allow**.

## Real-device tests

1. Enable USB debugging on an Android 9+ phone and confirm `adb devices` lists it.
2. Put `server-release-unsigned.apk` under `android/server/build/outputs/apk/release/` (or set `GS_SERVER_APK`).
3. Run:
   ```bash
   export GS_ADB_TEST_SERIAL=$(adb get-serialno)
   export GS_ADB_EXECUTABLE=$(which adb)
   build/posix-core/bin/gs_adb_device_tests
   build/posix-core/bin/gs_session_device_tests
   ```

For wireless debugging (Android 11+, same subnet), pair and connect first so Wi-Fi direct tests run:

```bash
adb pair <phone-wlan-ip>:<pairing-port> <6-digit-code>
adb connect <phone-wlan-ip>:<debug-port>
export GS_ADB_TEST_SERIAL=<phone-wlan-ip>:<debug-port>
```

If a phone VPN shows a VPN IP (such as `198.18.0.1`), use the WLAN IP from `adb shell ip -4 addr show wlan0`. Mac TUN proxies do not block same-subnet LAN traffic. Rebooting or switching Wi-Fi changes the wireless debugging port; run `adb connect` again without re-pairing.

Latest record (2026-11-24, Apple silicon macOS 27, Galaxy Z Fold8 Android 17, commit `5292501`, Debug):

- `gs_adb_device_tests` (wireless): 11 cases, 88 assertions passed.
- `gs_session_device_tests`: 15 cases passed on wireless (733 assertions, 2 runs) and USB (123 assertions).
- Median start-to-first-frame: 362 ms wireless (981 ms cold), 215 ms USB, 417 ms Wi-Fi direct only; 5 start/stop cycles held FDs at 45 and private memory at 11.3 to 11.5 MB.
- Not run in that pass: dual-phone streaming (`GS_ADB_TEST_OTHER_SERIAL`) and adb restart recovery (`GS_ADB_TEST_RESTART=1`).

## Platform and extension host mapping

`src/platform/src/posix` replaces Windows APIs:

| Header | Windows | macOS |
|:--|:--|:--|
| `TcpStream.h` | Winsock | BSD sockets (`SO_NOSIGPIPE`) |
| `UdpSocket.h` | Winsock + qWAVE | BSD sockets, DSCP CS5 + `NET_SERVICE_TYPE_VI`, self-datagram `interrupt()` |
| `Thread.h` | Thread description / priority | `pthread_setname_np` and QoS classes |
| `Crypto.h` | CNG, DPAPI | OpenSSL; AES-GCM with Keychain key |

Native extensions use `dylib` in `contributes.native` (see [Native extensions guide](extensions/native.md#macos)). `gs validate` checks Mach-O architecture, `gs_ext_main`, and imports. `src/exthost/src/posix/compat/windows.h` maps Win32 host primitives to POSIX:

| Windows | macOS |
|:--|:--|
| Event / File mapping | Auto-reset pipe / `shm_open` + `shm_unlink` |
| Wait multiple / `DuplicateHandle` | `kqueue` + `poll` / Unix socket `SCM_RIGHTS` (closing parent exits `gs-exthost`) |
| AppContainer / Minidump | `sandbox_init` (`posix/Container.cpp`) / `crash.txt` (`CrashHandler.cpp`) |

Covered by `gs_ext_tests`, `gs_mockhost_tests`, and `gs_exthost_tests`.
