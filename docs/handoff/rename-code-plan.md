# Renaming the code to anmacgamer (plan, 2026-11-26)

What users see is already anmacgamer (a7a6add: app, bundle, data folder, texts, help, companion label). This
plan covers the code itself: about 570 files contain `GameScrcpy` and 500 contain `gamescrcpy`. The user asked
for both; this splits the work by what each part can break, so the risky parts get a yes first.

## Tier A — internal only, nothing outside the repo notices (do first)

| What | Now | Becomes | Size |
|---|---|---|---|
| CMake target, test names that mention it | `GameScrcpy` | `anmacgamer` | `src/app/CMakeLists.txt`, tests |
| QML module URIs | `GameScrcpy.App`, `.Theme`, `.Controls`, `.Icons`, `.Blueprint` | `Anmacgamer.*` | ~500 `import` lines, `qt_add_qml_module` URIs, `qmllint` config |
| Translation files | `gamescrcpy_zh_CN.ts`, `gamescrcpy_en.ts` | `anmacgamer_*.ts` | CMake + loader |
| Resource file names | `GameScrcpy.icns`, `GameScrcpy.icon`, `GameScrcpy-1024.png` | `anmacgamer.*` | packaging, CMake, Info.plist |
| Java/Kotlin packages (namespace only) | `com.gamescrcpy.*` | `com.anmacgamer.*` | ~200 files, folders move |
| Copyright lines of our own files | "Yubo Wu and the GameScrcpy for macOS contributors" | "… the anmacgamer contributors" (done) | GPL-3.0 files only; Apache files keep upstream lines |
| Windows-only files | `GameScrcpy.rc.in`, `GameScrcpy.manifest`, `GameScrcpy.iss`, `.ico` | removed or renamed | the macOS build does not use them |

Checks: `cmake --preset mac-app`, full build, `ctest --preset mac-app`, `qmllint` 0 warnings, gradle
`:server:testDebugUnitTest :server:assembleRelease :companion:assembleDebug`, and a run on the phone.

## Tier B — names other people's files or programs use (needs the user's yes; keep reading the old name)

| What | Breaks | Proposed |
|---|---|---|
| Script API types `sdk/types/gamescrcpy.d.ts`, globals in it | every user's logic folder references `types/gamescrcpy.d.ts` | write `anmacgamer.d.ts`, keep `gamescrcpy.d.ts` as an alias for a release |
| Extension manifest key `"gamescrcpy": "2.2"` and the official id prefix `gamescrcpy.` | all existing extensions | accept both keys; new prefix `anmacgamer.` for official ones, old one still official |
| File format names `gamescrcpy.profile`, `gamescrcpy.settings`, `gamescrcpy.task`… | users' saved files and shared profiles | read both, write the new one |
| Link scheme `gamescrcpy://` | links people shared, extensions that open them | register both, generate the new one |
| MCP pipe `GameScrcpy-MCP` | AI clients the user already set up | listen on both names for a release |
| Keychain prefix `GameScrcpy/ai/` | saved AI keys | move keys over once on start |

## Tier C — phone side and protocol (recommend: do not change)

| What | Why keep |
|---|---|
| Android `applicationId` `com.gamescrcpy.companion` | changing it installs a second companion next to the old one; users would have to remove the old one and set up permissions again |
| Protocol constants (`"GameScrcpy companion pairing key 1"`, `"GameScrcpy companion computer"`, HMAC contexts, golden vectors) | phones already paired would stop connecting; they are never shown |
| Upstream credit (NuoRan/GameScrcpy, GameScrcpy-ESP32, licence notices) | the Apache-2.0 licence requires it |

## Status (2026-11-26 night)

Done: CMake target and project, QML module URIs (also Anmacgamer.Render), translation file names, copyright lines.
Open: Java/Kotlin packages (needs a phone run: the Mac names `com.gamescrcpy.server.Main` and the companion's
input method component), icon file names (README links; after Gemini's docs), Windows-only files, tier B.

## Order

1. Tier A in one commit per row (build and tests green after each).
2. Ask the user about Tier B; do it with the old names still read.
3. Tier C stays.
