# Voice commands (`voice-commands`)

[简体中文](README.md) · English

Demonstrates `speech.listen` (listening for voice commands), `input.source` (sending bound keys to the front mirror window), and `speech.say` (spoken replies). Needs `speech.listen` and `hardware`.

## Try it

1. Validate, check, and pack:
   ```powershell
   gs validate examples\extensions\voice-commands
   gs check examples\extensions\voice-commands
   gs pack examples\extensions\voice-commands -o voice-commands.gsext
   ```
2. Install via **Install from file…**, enable it, and approve permissions.
3. Open **Voice commands** in the sidebar to choose engine, language, confidence, and edit commands (defaults: "open map", "open bag", "reload" in English; "打开地图", "打开背包", "换弹" in Chinese; saved in `store`).
4. In the key mapping editor, bind `Ext.example.voice-commands.voice.map` by saying "open map" while waiting for a key. Saying it in the mirror window presses the key for 80 ms and speaks the reply (`interrupt: true`).
