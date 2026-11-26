The key mapping editor lets you place keys onto game buttons directly over the live phone screen.

![The key mapping editor](figure:editor-overview)

## Open the editor and add keys

1. **Open the editor**: in the mirror window, open **Key mapping options > Edit key mapping** (or click **Create and edit** when making a new profile).
2. **Pick a component**: on the right panel's **Library** tab, find a component (use **Search components** or hover for details).
3. **Place it**: drag the component onto a game button, or select it and press <kbd>Enter</kbd> to place it in the center.
4. **Bind a key**: press a keyboard key, mouse button, or scroll wheel, or click **Skip** to bind later.

![Add a key](figure:editor-components,editor-drag!,editor-capture)

## Built-in components

| Group | Component | Action |
|---|---|---|
| Basics | Tap | Taps once per key press. |
| | Hold | Holds the touch while the key is held. |
| | Long press | Holds for a set duration per press. |
| | Turbo | Taps repeatedly at a set rate while held. |
| | Swipe | Swipes along a drawn path. |
| | Pinch | Two keys spread or pinch two fingers. |
| | Multi-press | Holds multiple spots at once. |
| Movement | WASD stick | Moves the on-screen stick with WASD, walk, and sprint keys. |
| | Right-click walk | Click to walk toward that spot. |
| | Gamepad stick | Maps a gamepad left stick to the on-screen stick. |
| Wheels | Direction wheel | Opens a game wheel and drags toward the pointer. |
| | Action wheel | Opens a custom wheel to trigger components, spots, or logic. |
| View | View | Turns the camera with the mouse while locked. |
| | Gamepad view | Maps a gamepad right stick to camera movement. |
| | Free look | Looks around while held without turning the character. |
| | Temporary cursor | Shows the cursor while held during camera lock. |
| MOBA | Skill cast | Aims toward the pointer on press; casts on release or click. |

System actions (Back, typing, screenshots, switching profiles) are handled in [Scripts](help:scripts).

## Example & custom components

- **Example components** (**Toggle hold**, **Double tap**, **Burst**, **Tap in turn**, **Show all key hints while held**) are built with logic. Double-click one to inspect it, or click **Copy to my components** to customize your own copy.
- **My components**: click **New component…** at the bottom of the library, pick a base (scratch, built-in, or custom), and edit its logic. Custom components are shared across all profiles that use them and reload on save.
- **Groups & exports**: components with child components drag onto the screen as a group. Placing a custom component turns the profile into a folder profile with logic.

## Arrange, edit, and resolve conflicts

- **Select & move**: click to select, <kbd>⌘ Command</kbd>-click or <kbd>Shift</kbd>-click to multi-select, or drag on empty space to box-select. Drag to move (snaps to other keys and center; hold <kbd>⌥ Option</kbd> to place freely), or nudge with arrow keys (<kbd>Shift</kbd>+Arrow moves faster).
- **Copy & paste**: press <kbd>⌘ Command</kbd>+<kbd>C</kbd> and <kbd>⌘ Command</kbd>+<kbd>V</kbd> to paste at the pointer (even across different phone editors).
- **Freeze & zoom**: click **Freeze the picture** to pause moving screens while placing keys. Zoom with the toolbar buttons or <kbd>⌘ Command</kbd>+scroll; click the percentage to reset.
- **Properties**: select a single component to change its key, sensitivity/timing, **Anchor** corner (keeps relative distance across screen sizes), **On at start**, **Code name** (used in logic as `controls.<name>`), **Applies when** condition, extension **Add-ons**, and **Use it in logic**.
- **Logic-claimed keys**: if logic listens to a component's key events, a badge appears on its keycap and only logic runs unless you set **When the key is pressed** to **Press it as usual, then run the logic**.
- **Conflicts**: when two components share a key, the top bar shows the conflict count; click it to select them and rebind one.

![Two keys bound to Q: the top bar flags the conflict and highlights both keys.](figure:editor-conflict)

- **Delete**: press <kbd>Delete</kbd> or **More > Delete**. If logic references the component, choose **Delete those blocks too**, **Delete only the component** (broken blocks can be replaced later in the blueprint), or **Cancel**.

## Test, preview, and save

- **Try**: click **Try** (flask icon) on the top bar to test keys on the live game without leaving the editor; click again to resume editing.
- **Preview screens**: use **Screen shape to preview** to check layout on other aspect ratios (such as 20:9 or 21:9); switch back to the phone's own shape to edit.
- **Picture / Logic**: switch between **Picture** and **Logic** in the middle of the top bar.
- **Save**: click **Save** to write changes, **Done** to save and exit, **More > Save as…** to clone, or **More > Discard changes and exit** to cancel. For extension profiles, **More > Restore original…** reverts your edits.

![While trying keys, the dim overlay clears and keys control the game live.](figure:editor-try)

## AI drafts

When an AI client edits a profile over MCP, its card shows a button like **Claude's draft**. Opening it launches a read-only preview highlighting added (green), changed (orange), and removed (red) keys and logic diffs. Click **Apply** to accept, **Discard** to delete, or **Close** to decide later.

## Shortcuts

| Keys | Action |
|---|---|
| <kbd>⌘ Command</kbd>+<kbd>Z</kbd> | Undo |
| <kbd>⌘ Command</kbd>+<kbd>Y</kbd> | Redo |
| <kbd>⌘ Command</kbd>+<kbd>S</kbd> | Save |
| <kbd>⌘ Command</kbd>+<kbd>D</kbd> | Duplicate selection |
| <kbd>⌘ Command</kbd>+<kbd>A</kbd> | Select all |
| <kbd>Delete</kbd> | Delete selection |

Customize these in **Settings > Keyboard shortcuts**.
