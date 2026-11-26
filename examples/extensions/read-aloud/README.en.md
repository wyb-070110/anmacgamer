# Read aloud (`read-aloud`)

[简体中文](README.md) · English

Demonstrates a session script (`"run": "session"`) with a mirror toolbar button (`read`) and side panel (`area`): reads text from a screen region via `vision.recognize("system", "ocr", …)` and speaks it via `speech.say`. Needs `device.screen`.

## Try it

1. Validate, check, and pack:
   ```powershell
   gs validate examples\extensions\read-aloud
   gs check examples\extensions\read-aloud
   gs pack examples\extensions\read-aloud -o read-aloud.gsext
   ```
2. Install via **Install from file…**, enable it, and approve `device.screen`.
3. Open a mirror window (or use `GameScrcpy.exe --demo-device tests\data\video\emulator_h265.media`).
4. Adjust Left, Top, Width, Height, and language in the **Read aloud** side panel (saved in `store`), then click the speaker toolbar button or **Read now**.

`speech.say(text, { interrupt: true })` needs no permission and interrupts any prior utterance. `ui.toolbar("read").set({ checked: true })` marks the button while speaking.
