# Key list (`key-list`)

[简体中文](README.md) · English

Demonstrates `contributes.converters` by importing `.keys` plain-text files as profiles and exporting profiles back to `.keys`. Needs no permissions.

## Format

```text
# Arena basics
J 0.82 0.71 Attack
Space 0.94 0.80 Dodge
1 0.40 0.92 Potion
```

Line 1 starting with `#` sets the profile name; other `#` lines are comments. Each entry is `key x y [name]` (`x`/`y` in `0..1`). Import builds tap controls; export writes single-key positioned controls and comments the rest.

## Try it

1. Validate, check, and pack:
   ```zsh
   gs validate examples/extensions/key-list
   gs check examples/extensions/key-list
   gs pack examples/extensions/key-list -o key-list.gsext
   ```
2. Install via **Install from file…** and enable it.
3. On the Profiles page, click **Import** and choose `samples/arena.keys` (or drop it on the mirror window) to create "Arena basics".
4. Open a profile card's menu and choose **Export as Key list…**.

Both converters share `src/main.ts` (`convert(input)` branches on `input.to`, returning `{ name, json }` for `gamescrcpy.profile` v3 on import or `{ text }` on export).
