A profile saves the key mappings, logic, and vision resources for a game. Manage all profiles on the **Profiles** page.

## Create a profile

1. On the **Profiles** page, click **New profile…** (or pick a choice from the prompt when opening an unmapped game).
2. Choose a starting point:
   - :layout-grid: **Use a template**: pick Shooter, MOBA, Racing, Action / RPG, Cards / Tapping, or Blank, name it, link it to a game, and set **Landscape** or **Portrait**.
   - :puzzle: **Community profiles**: start from an extension-provided profile.
   - :plus: **Start blank**: place every key yourself.
3. Click **Create and edit** to open the [Key mapping editor](help:keymap-editor) on a mirrored phone.

> [!NOTE]
> **Built-in Honkai: Star Rail profiles**: the mouse works as a normal pointer, <kbd>Tab</kbd> locks the camera while exploring, and <kbd>F1</kbd>–<kbd>F5</kbd> require holding <kbd>fn</kbd> on Mac keyboards unless **Use F1, F2, etc. keys as standard function keys** is enabled in macOS System Settings.

## Switch and link profiles

- **Switch manually**: open **Key mapping options** on the mirror toolbar to pick a profile, or click **Use on** on a profile card.
- **Auto-switch by game**: open **More (…)** on a profile card and select **Link to …** for a running game. With **Settings > Input & key mapping > Switch profiles with the game** on, anmacgamer loads it automatically whenever that game opens.

![Linked profiles switch automatically when the game opens.](figure:profile-link)

## Organize & edit extension profiles

- Search by profile or game name, or filter by **All**, **This game**, **Mine**, **From extensions**, and **Problems**.
- Use **Duplicate** to copy, **Delete…** to move to Trash, or **Show in folder** / **Open folder** to view files in Finder.
- Extension profiles are marked **Read-only**, but your edits are saved as overrides. Use **Restore original…** to revert, or **Copy to mine** to make an editable copy.

## Export and import

A profile is either a `.json` file (keys only) or a profile folder (keys, `logic.ts`/`logic.js`, and `res/` images).

![Export a profile folder as a single .gsprofile file to share.](figure:profile-share)

- **Export**: open **More (…) > Export…** on a card to save a `.gsprofile` bundle (includes logic and images) or `.json`.
- **Import**: click **Import…** on the **Profiles** page, or drag `.json` / `.gsprofile` files onto the window. Converter extensions add support for extra formats (see [What extensions add](help:extension-features#converters)).

> [!WARNING]
> Only import profiles with logic from sources you trust—logic can send touches, read key states, and see pointer positions.
