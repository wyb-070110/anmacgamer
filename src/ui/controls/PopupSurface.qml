import QtQuick
import QtQuick.Effects
import Anmacgamer.Theme

Item {
    id: surface

    property real radius: Theme.radius.lg
    property bool deep: true

    RectangularShadow {
        visible: surface.deep
        anchors.fill: face
        radius: face.radius
        offset.y: Theme.shadow.ambientOffset
        blur: Theme.shadow.ambientBlur
        color: Theme.shadow.ambient
    }

    RectangularShadow {
        anchors.fill: face
        radius: face.radius
        offset.y: Theme.shadow.keyOffset
        blur: Theme.shadow.keyBlur
        color: Theme.shadow.key
    }

    // Liquid Glass: a tinted pane with a soft sheen from the top and a lit top edge.
    Rectangle {
        id: face

        anchors.fill: parent
        radius: surface.radius
        color: Theme.colors.bgElevated
        border.width: Theme.size.stroke
        border.color: Theme.liquid ? Theme.colors.strokeDefault : Theme.colors.strokeStrong

        Rectangle {
            anchors.fill: parent
            anchors.margins: Theme.size.stroke
            radius: Math.max(0, face.radius - Theme.size.stroke)
            visible: Theme.liquid
            gradient: Gradient {
                GradientStop { position: 0; color: Theme.dark ? "#14FFFFFF" : "#66FFFFFF" }
                GradientStop { position: 0.4; color: "#00FFFFFF" }
            }
        }

        Rectangle {
            x: face.radius
            y: Theme.size.stroke
            width: Math.max(0, face.width - 2 * face.radius)
            height: Theme.size.stroke
            visible: Theme.liquid
            color: Theme.colors.glassEdge
        }
    }
}
