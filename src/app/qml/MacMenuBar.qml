// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
pragma ComponentBehavior: Bound

import QtQuick
import Qt.labs.platform as Platform
import Anmacgamer.App

// The macOS menu bar: the app's commands (the same list as the command palette and the shortcuts) in the menus a
// Mac app has, so names, translations, shortcuts and whether an item can be used come from one place. Device items
// act on the device used last. Its shortcut is set only while an item can be used, so a key the command cannot use
// still reaches the window.
Platform.MenuBar {
    id: bar

    required property Window hub

    component CommandItem: Platform.MenuItem {
        id: item

        required property string command

        readonly property CommandState state: CommandState {
            command: item.command
        }

        text: state.title
        enabled: state.enabled
        checkable: state.checked
        checked: state.checked
        shortcut: state.enabled ? state.shortcut : ""
        onTriggered: state.trigger()
    }

    Platform.Menu {
        title: qsTr("anmacgamer")

        CommandItem {
            command: "app.about"
            role: Platform.MenuItem.AboutRole
        }
        CommandItem {
            command: "hub.settings"
            role: Platform.MenuItem.PreferencesRole
        }
    }

    Platform.Menu {
        title: qsTr("File")

        CommandItem { command: "wireless.pair" }
    }

    Platform.Menu {
        title: qsTr("View")

        CommandItem { command: "hub.devices" }
        CommandItem { command: "hub.profiles" }
        CommandItem { command: "hub.tasks" }
        CommandItem { command: "hub.extensions" }
        Platform.MenuSeparator {}
        CommandItem { command: "app.commandPalette" }
        CommandItem { command: "app.assistant" }
    }

    Platform.Menu {
        title: qsTr("Device")

        CommandItem { command: "device.mirror" }
        CommandItem { command: "device.showWindow" }
        CommandItem { command: "device.disconnect" }
        Platform.MenuSeparator {}
        CommandItem { command: "device.back" }
        CommandItem { command: "device.home" }
        CommandItem { command: "device.appSwitch" }
        CommandItem { command: "device.notifications" }
        CommandItem { command: "device.quickSettings" }
        Platform.MenuSeparator {}
        CommandItem { command: "device.screenOff" }
        CommandItem { command: "device.mute" }
        CommandItem { command: "mirror.record" }
        CommandItem { command: "device.files" }
        CommandItem { command: "device.measureLatency" }
        Platform.MenuSeparator {}
        CommandItem { command: "device.keymap" }
        CommandItem { command: "device.editKeymap" }
        CommandItem { command: "device.fullscreen" }
        CommandItem { command: "device.mini" }
        Platform.MenuSeparator {}
        CommandItem { command: "device.settings" }
    }

    Platform.Menu {
        title: qsTr("Window")

        Platform.MenuItem {
            text: qsTr("Minimize")
            shortcut: "Ctrl+M"
            onTriggered: bar.hub.showMinimized()
        }
        Platform.MenuItem {
            text: qsTr("Zoom")
            onTriggered: bar.hub.visibility === Window.Maximized ? bar.hub.showNormal() : bar.hub.showMaximized()
        }
        Platform.MenuSeparator {}
        Platform.MenuItem {
            text: qsTr("anmacgamer")
            onTriggered: {
                bar.hub.show()
                bar.hub.raise()
                bar.hub.requestActivate()
            }
        }
    }

    Platform.Menu {
        title: qsTr("Help")

        CommandItem { command: "app.help" }
        CommandItem { command: "app.guide" }
    }
}
