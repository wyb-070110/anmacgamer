# Deck bridge (`deck-bridge`)

[简体中文](README.md) · English

Demonstrates `net.listen` (local HTTP server on `127.0.0.1`), `input.source` (feeding keys into key mapping as `deck`), and `ext.provide` (services for other extensions). Needs `network.listen` and `hardware`.

## Try it

1. Validate, check, and pack:
   ```zsh
   gs validate examples/extensions/deck-bridge
   gs check examples/extensions/deck-bridge
   gs pack examples/extensions/deck-bridge -o deck-bridge.gsext
   ```
2. Install via **Install from file…** on the Extensions page, enable it, and approve permissions.
3. Open **Deck bridge** in the sidebar (`Listening on http://127.0.0.1:8765`).
4. In the key mapping editor, bind a key while running:
   ```zsh
   curl -fsS -X POST http://127.0.0.1:8765/tap/k1
   ```
   This records `Ext.example.deck-bridge.deck.k1`. Send the same `POST` from Stream Deck or OBS; keys target the front mirror window.

## Endpoints and services

| Request | Action |
|:--|:--|
| `GET /` | Returns `{ listening, port, held }` |
| `POST /tap/<key>?ms=80` | Tap `<key>` for `ms` (10–2000, default 80) |
| `POST /press/<key>` / `POST /release/<key>` | Hold or release `<key>` (`[a-zA-Z0-9_-]{1,32}`) |

Mutations require `POST`. Other extensions depending on `example.deck-bridge` can call `ext.call("example.deck-bridge/status")` or `ext.call("example.deck-bridge/tap", { key: "k1", ms: 120 })` (`tap` requires `hardware`). Turning off local connections in Details stops the server; press **Listen** to restart it.
