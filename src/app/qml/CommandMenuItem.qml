import QtQuick
import Anmacgamer.Controls
import Anmacgamer.App

MenuItem {
    required property CommandState commandState

    visible: commandState.enabled
    height: visible ? implicitHeight : 0
    iconName: choice ? "" : commandState.icon
    chosen: commandState.checked
    text: commandState.title
    shortcut: commandState.shortcut
    onTriggered: commandState.trigger()
}
