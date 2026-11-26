import QtQuick
import Anmacgamer.Theme

Item {
    id: button

    required property string glyph
    property bool destructive: false

    signal clicked()

    // macOS: the window's own traffic lights minimize, zoom and close it; these Windows caption buttons are hidden.
    visible: false
    implicitWidth: 0
    implicitHeight: 40

    Rectangle {
        anchors.fill: parent
        color: {
            if (!mouse.containsMouse)
                return "transparent"
            if (button.destructive)
                return mouse.pressed ? Qt.darker(Theme.colors.danger, 1.15) : Theme.colors.danger
            return mouse.pressed ? Theme.colors.strokeStrong : Theme.colors.strokeDefault
        }

        Behavior on color {
            ColorAnimation {
                duration: Theme.motion.fast
                easing.type: Easing.BezierSpline
                easing.bezierCurve: Theme.motion.standard
            }
        }
    }

    Text {
        anchors.centerIn: parent
        textFormat: Text.PlainText
        text: button.glyph
        font.family: Theme.type.iconFamily
        font.pixelSize: 10
        color: button.destructive && mouse.containsMouse ? "#FFFFFF" : Theme.colors.textSecondary
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onClicked: button.clicked()
    }
}
