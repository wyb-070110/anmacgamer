pragma ComponentBehavior: Bound

import QtQuick
import Anmacgamer.Theme
import Anmacgamer.App

Item {
    id: titleBar

    required property Window window
    property WindowChrome chrome: null

    readonly property Item minimizeButton: minimizeButton
    readonly property Item maximizeButton: maximizeButton
    readonly property Item closeButton: closeButton

    implicitHeight: 40

    // macOS: the system's traffic lights sit at the left of the bar; dragging past the drag threshold hands window
    // movement to AppKit (performWindowDragWithEvent:), while double-clicking follows the macOS title bar preference.
    MouseArea {
        property real pressX: 0
        property real pressY: 0
        property bool moving: false

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onPressed: mouse => {
            pressX = mouse.x
            pressY = mouse.y
            moving = false
        }
        onPositionChanged: mouse => {
            if (!pressed || moving)
                return
            if (Math.hypot(mouse.x - pressX, mouse.y - pressY) >= Application.styleHints.startDragDistance) {
                moving = true
                if (titleBar.chrome !== null)
                    titleBar.chrome.startDrag()
                else
                    titleBar.window.startSystemMove()
            }
        }
        onDoubleClicked: {
            if (titleBar.chrome !== null) {
                titleBar.chrome.doubleClickTitleBar()
            } else if (titleBar.window.visibility === Window.Maximized) {
                titleBar.window.showNormal()
            } else {
                titleBar.window.showMaximized()
            }
        }
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 78
        anchors.verticalCenter: parent.verticalCenter
        textFormat: Text.PlainText
        text: titleBar.window.title
        color: Theme.colors.textTertiary
        font.pixelSize: Theme.type.caption.size
        font.weight: Font.Medium
    }

    Row {
        visible: false
        anchors.right: parent.right
        anchors.top: parent.top
        height: parent.height

        CaptionButton {
            id: minimizeButton
            glyph: "\uE921"
            onClicked: titleBar.window.showMinimized()
        }

        CaptionButton {
            id: maximizeButton
            glyph: titleBar.window.visibility === Window.Maximized ? "\uE923" : "\uE922"
            onClicked: {
                if (titleBar.window.visibility === Window.Maximized)
                    titleBar.window.showNormal()
                else
                    titleBar.window.showMaximized()
            }
        }

        CaptionButton {
            id: closeButton
            glyph: "\uE8BB"
            destructive: true
            onClicked: titleBar.window.close()
        }
    }
}
