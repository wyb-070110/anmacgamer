# G10 — Rewrite the in-app tutorials: shorter and easy to follow (2026-11-26)

The user: the tutorials in the app are too long and hard to understand; rewrite all of them.

## What to rewrite

1. **Help articles**, both languages: `src/app/help/en/*.md` and `src/app/help/zh_CN/*.md` (13 each:
   aoa-driver, capture-card, companion, connecting, extension-features, extensions, faq, keymap-editor,
   mirror-window, profiles, scripts, touch-box, tours), and the `title` / `summary` / `meta` texts in
   `src/app/help/index.json`.
2. **The first-run guide and the page tours**: their texts are in QML (`src/app/qml/GuideOverlay.qml`,
   `TourOverlay.qml`, and the `tour` step lists in `DevicesPage.qml`, `ProfilesPage.qml`, `ExtensionsPage.qml`,
   `MirrorOverlay.qml`, `KeymapEditor.qml`, `LogicView.qml`, `AiCorner.qml`, `Main.qml`). **Do not edit QML.** Write
   the new English and Chinese wording for every step into `docs/handoff/gemini/g10-guide-tours.md` (file, step,
   old English, new English, new Chinese); Claude puts them in the code and the translation files.

## How it should read

- For someone who has never used it: what to do, in order, in short steps. One idea per sentence. No background
  unless it is needed to act.
- **Aim for about half the current length.** Cut repetition, edge cases that rarely happen (move them to the FAQ
  if they matter), and explanations of how things work inside.
- Start each article with one sentence saying what it is for, then the steps.
- English first; the Chinese says the same thing in natural Chinese (not a word-for-word translation). Both files
  of a pair keep the same sections in the same order.
- No marketing words, no emoji, no "simply/just/easily", no bold on every line.
- `scripts.md` and `extension-features.md` are reference pages for people who write logic and extensions: make
  them easier to scan (short intro, a short "first script" walk-through, then the reference), but keep every API
  name, parameter and rule that is there; do not invent API.
- Only describe what the app does now. If unsure whether something exists, keep the current wording for it and
  list it under "unsure" in your notes.

## Markup that must keep working (the app parses it; `tests/unit/app/HelpMarkdownTests.cpp` and
`HelpLibraryTests.cpp` check it)

- Headings may end with `{#anchor}`, `{icon=name}` or a short tag like `{Only once}`. **Keep every existing
  `{#anchor}` exactly**; these are linked from elsewhere: `connecting#network`, `extension-features#converters`,
  `extension-features#media`, `extensions#trouble`, `faq#no-input`, `faq#no-phone`, `faq#no-sound`,
  `faq#pointer-gone`, `tours#extension-tours`. Headings without an anchor get one from their text, so if you rename
  such a heading, search the help for links to the old one.
- Figures: `![caption](figure:name1,name2!)` — keep the figure names as they are (they are drawings in the app):
  camera-lock, device-stepper, editor-capture, editor-components, editor-conflict, editor-drag, editor-overview,
  editor-try, mirror-overview, mirror-toolbar, pair-qr, phone-allow, phone-pairing, phone-ready, phone-scan,
  phone-usb, phone-usbmode, profile-link, profile-share, script-workbench, setup-steps, until-flow, vision-draw,
  vision-freeze, vision-name, vision-test, window-modes. You may drop a figure; do not make up new names.
- Actions and links: `[text](action:guide)`, `[text](action:tour:devices)` (and the other tours),
  `[text](help:slug#anchor)`. Keep the targets valid.
- Callouts: `> [!TIP]`, `> [!NOTE]`, `> [!WARNING]`, `> [!IMPORTANT]`, `> [!CAUTION]`.
- Numbered steps written as `1. **Short action**: detail.` render as step cards next to a figure.

## Facts to keep right (changed recently)

- The app is **anmacgamer**; the phone app is **anmacgamer Companion** (伴侣). Upstream credit to NuoRan/GameScrcpy
  stays where it is.
- Without USB debugging: the phone on the cable is controlled over AOA; with the companion paired by QR code over
  Wi-Fi, the companion also sends the picture and sound (same Wi-Fi needed), and the phone stays silent while
  mirroring. One QR scan per computer.
- The companion app is one Settings-style page now (status, Features, Computers, Permissions, About) — no tabs.
- Phone sound is controlled from the Mac; the phone's volume keys do not change it while mirroring.
- Toolbar of the mirror window: Back, Home, Recents fixed, plus up to 5 buttons you choose; Keep on top is one
  of them.
- Built-in Star Rail profiles: the mouse is a normal pointer; Tab locks the view while exploring; F keys need
  fn on Mac keyboards unless "Use F1, F2, etc. keys as standard function keys" is on.

## Deliver

- Edit the files above directly (do not commit; Claude reviews, builds, runs the tests and commits).
- Write `docs/handoff/gemini/g10-guide-tours.md` (the QML wording) and a short note at the end of
  `docs/handoff/gemini/README.md`: files changed, old and new word counts, anything you were unsure about.
- Do not touch anything else (no `src/` code, no `docs/` outside `docs/handoff/gemini/`, no `TEAM.md`).
