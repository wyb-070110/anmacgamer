Extensions add features directly into the pages and menus where you use them. Open an extension's **Details** on the **Extensions** page to see everything it provides; turning the extension off hides all of its additions.

See [Extensions](help:extensions) to install, enable, and update packages.

## Devices page {#devices}

- :plug: **Touch boxes**: each box appears as its own card with status, firmware actions, and **Control** / **Settings** / **More**. See [Touch boxes](help:touch-box).
- :link: **Connection methods**: phones connected via an extension appear on **Devices** with that method's icon.
- :monitor-smartphone: **Picture sources**: external video/audio sources appear like capture cards. See [Capture cards](help:capture-card).

## Mirror window & recordings {#media}

- :share: **Send elsewhere**: video/audio sinks (such as virtual cameras) appear under **More > Send to …** in the mirror window.
- :history: **Export recordings**: recording exporters add **Export as …** to each clip's **More** menu under **More > Recordings…**.
- :eye: **Picture readers**: frame-reading extensions appear under **More > Extensions reading the picture** (toggled per phone).

## Reading aloud & voice commands {#speech}

Scripts can speak with `speech.say` and listen with `speech.listen`. Configure macOS or extension speech engines under **Settings > Audio > Reading aloud and voice commands**:

- :volume-2: **Reading aloud**: pick **Speech engine**, **Voice**, **Speaking rate**, **Speaking volume**, and test with **Read aloud**.
- :audio-lines: **Voice commands**: pick **Voice command engine** and **Voice command language**.

> [!NOTE]
> If macOS reports missing voices or recognition, download voices in **System Settings > Accessibility > Spoken Content** and allow Microphone and Speech Recognition in **System Settings > Privacy & Security**.

Extension scripts require the "Listen for voice commands you say" permission to use `speech.listen`.

## Interface languages {#languages}

Language pack extensions add options under **Settings > Appearance > Language > Interface language** (shown as `language · extension`). Untranslated help articles fall back to English.

## Files and folders for logic {#files}

Logic must ask you before accessing local files or folders:

- **Single files**: prompts you to pick one file to open or save; cancelling returns nothing.
- **Folders**: remembers folders you approve. Manage or **Forget** them under **Settings > General > Folders your own logic may use** (or in an extension's **Settings**).

## AI model services {#ai}

AI provider extensions appear under **Settings > AI > AI model > Model service** as `service (extension)` with no manual URL or API key needed—click **Get** on **Model** to list available models for the assistant and `ai.ask`.

## Import & export converters {#converters}

- **Import**: click **Import…** on **Profiles** (or drop a file onto the window) to import supported formats as profiles.
- **Export**: adds **Export as …** to the **More** menu on profile cards, task settings, and recordings.

Files over 32 MiB or conversions that time out are stopped.

## Panels, components & more {#more}

- **Panels**: appear beside the mirror window (**Extension panels**), as main window pages, in Settings, in card **More** menus, or in the menu bar.
- **Components & add-ons**: appear in the keymap editor library and component **Add-ons** (global toggles live in extension **Settings**).
- **Input devices & recognition**: adds custom hardware buttons to the keymap editor and OCR/object engines to logic.
- **Local connections**: extensions that accept connections from local apps can be toggled under **Details > Accept connections from programs on this computer**.
- **Help & tours**: extension help articles appear in Help under the extension's name; tours launch from **Details**. See [Guided tours](help:tours#extension-tours).
