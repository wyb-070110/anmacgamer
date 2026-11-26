# Notes (`notes-panel`)

[简体中文](README.md) · English

Demonstrates `files.folder` (storing `.txt` notes in a user-chosen folder) and `net.fetch` (fetching the latest release from `api.github.com`) on a main-window page. Needs `files.paths` (`notes`) and `network` (`api.github.com`).

## Try it

1. Validate, check, and pack:
   ```powershell
   gs validate examples\extensions\notes-panel
   gs check examples\extensions\notes-panel
   gs pack examples\extensions\notes-panel -o notes-panel.gsext
   ```
2. Install via **Install from file…**, enable it, and approve permissions.
3. Open **Notes** in the sidebar and pick a folder on first open (you can forget it in extension settings).
4. Add, read, reload, or remove `.txt` notes, or click **Add the newest GameScrcpy release** to fetch from `api.github.com` (other hosts are blocked with `GS-API-155`).
