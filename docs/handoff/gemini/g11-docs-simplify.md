# G11 — Make the project's Markdown docs shorter and plain (2026-11-26)

The user: the project has many Markdown documents; cut them down and write them so a person understands them easily.
A second Gemini task (G10) is rewriting the in-app help at the same time: **do not touch `src/app/help/`**.

## In scope (edit these)

| Group | Files | Note |
|---|---|---|
| Front door | `README.md`, `README.zh-CN.md`, `CONTRIBUTING.md`, `CONTRIBUTING.zh-CN.md`, `SECURITY.md`, `docs/README.md` | the most read; make them the clearest |
| Project docs | `docs/macos-port.md`, `docs/architecture.md` | for contributors |
| Extension guides | `docs/extensions/README.md`, `logic.md`, `manifest.md`, `native.md`, `publishing.md`, `testing.md`, and their `.zh-CN.md` twins | reference for extension authors; see below |
| Example extensions | `examples/extensions/*/README.md` and `README.en.md` | short: what it shows, how to try it |
| Release notes not yet published | `packaging/macos/RELEASE-NOTES-beta3.md` | beta1/beta2 are published; leave them |
| Changelog | `CHANGELOG.md` | only the macOS sections (2.2.1 betas); older upstream entries stay as they are |

## Out of scope (do not edit)

`protocol/SPEC.md` and `protocol/vectors/README.md` (exact specification), `LICENSING.md` and `CODE_OF_CONDUCT.md`
(legal and covenant text), `AGENTS.md`, `HANDOFF.md`, `TEAM.md`, `.agents/`, everything in `docs/handoff/`,
`src/` (including `src/app/help/`), `sdk/` (generated), `tests/`, `examples/extensions/tour-and-help/help/` (it is
help content checked by a test), `third_party/`.

## How it should read

- Say what the reader can do and how, in short sentences. One idea per sentence. Plain words; explain a term the
  first time it appears or drop it.
- Lead with the most useful part: what it is, how to install or start, then details. Put rare cases at the end
  or in a short "If something goes wrong" section.
- **Aim for roughly half the length** of the front-door and project docs. Remove repetition, history, and
  "how it works inside" unless the page is about that (architecture).
- Extension guides are references: keep every field, rule, limit, API name and error code that is there — a
  wrong or missing rule breaks people's extensions. Make them easier to scan instead: a short intro, a "smallest
  working example" first, then tables. Cut wordy explanations around the facts, not the facts.
- English first; the `.zh-CN.md` twin says the same in natural Chinese, same sections in the same order.
- No marketing words, no emoji, no "simply/just/easily/powerful/seamless", no bold on every line.
- Keep links working: if you rename a heading, fix links to its anchor; keep code blocks and commands exact.

## Facts that must stay right

- The app is **anmacgamer** (`anmacgamer.app`), the phone app **anmacgamer Companion**; repository
  `wyb-070110/anmacgamer`. Upstream credit to NuoRan/GameScrcpy (Apache-2.0) and the licence statements stay
  word for word.
- macOS 14 or later, Apple silicon and Intel (universal). Android 9 or later.
- Build: `cmake --preset mac-app`, `cmake --build --preset mac-app`, `ctest --preset mac-app`; release preset
  `mac-release`; Android: `cd android && ./gradlew build`.
- Do not add features that are not in the code; when unsure, keep the current sentence and list it in your notes.

## Deliver

- Edit the files directly; do not commit (Claude reviews and commits).
- Write your notes to `docs/handoff/gemini/g11-notes.md` (not the shared README, G10 writes there): files changed,
  old and new word counts, anything you were unsure about.
