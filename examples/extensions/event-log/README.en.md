# Event log (`event-log`)

[简体中文](README.md) · English

Demonstrates `events.on` by subscribing to all app and device event topics and listing them newest-first on a main-window page (with pause, filter, and clear). Needs `events.app` for phone/window events.

## Try it

1. Validate, check, and pack:
   ```powershell
   gs validate examples\extensions\event-log
   gs check examples\extensions\event-log
   gs pack examples\extensions\event-log -o event-log.gsext
   ```
2. Install via **Install from file…**, enable it, and approve `events.app`.
3. Open **Event log** in the sidebar, then connect a phone, start mirroring, switch profiles, or toggle an extension to watch rows appear.

Without `events.app`, an app script only receives `extension.enabled`, `extension.disabled`, and `settings.changed`. See payload schemas in [sdk/docs/events.md](../../../sdk/docs/events.md).
