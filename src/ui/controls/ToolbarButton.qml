import QtQuick
import QtQuick.Templates as T
import Anmacgamer.Theme
import Anmacgamer.Icons

T.AbstractButton {
    id: control

    property string iconName
    property string iconPath
    property string shortcut
    property bool selected: checked
    property bool compact: false
    property bool alert: false
    // The side its tooltip opens on: the popupEdge of the FloatingToolbar it is in, below otherwise.
    readonly property string popupEdge: {
        for (let item = parent; item; item = item.parent) {
            if (item.vertical !== undefined && item.popupEdge !== undefined)
                return item.popupEdge
        }
        return "bottom"
    }

    readonly property color foreground: {
        if (selected)
            return alert ? Theme.colors.dangerText : Theme.colors.accentText
        return hovered ? Theme.colors.textPrimary : Theme.colors.textSecondary
    }

    readonly property bool across: parent instanceof Grid ? parent.columns === 1 : parent instanceof Column

    implicitWidth: compact && !across ? Theme.size.toolbarChevron : Theme.size.toolbarButton
    implicitHeight: compact && across ? Theme.size.toolbarChevron : Theme.size.toolbarButton
    padding: 0
    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    opacity: enabled ? 1 : Theme.disabledOpacity

    Accessible.name: text
    Accessible.description: shortcut
    Accessible.checkable: checkable || selected
    Accessible.checked: selected

    Tooltip {
        objectName: "toolTip"
        x: control.popupEdge === "right" ? control.width + Theme.space.xs
         : control.popupEdge === "left" ? -width - Theme.space.xs : (control.width - width) / 2
        y: control.popupEdge === "bottom" ? control.height + Theme.space.xs
         : control.popupEdge === "top" ? -height - Theme.space.xs : (control.height - height) / 2
        text: control.shortcut !== "" ? qsTr("%1 (%2)").arg(control.text).arg(Theme.keyLabel(control.shortcut)) : control.text
        visible: control.text !== "" && control.hovered && !control.down
    }

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: control.iconName
            customPath: control.iconPath
            size: control.compact ? Theme.size.iconSmall : Theme.size.icon
            color: control.foreground
        }
    }

    background: Rectangle {
        id: face

        radius: Math.min(width, height) / 2
        color: control.selected ? (control.alert ? Theme.colors.dangerSubtle : Theme.colors.accentSubtle) : "transparent"

        StateLayer {
            anchors.fill: parent
            radius: face.radius
            hovered: control.hovered
            pressed: control.down
        }

        FocusRing {
            visible: control.visualFocus
            baseRadius: face.radius
        }
    }
}
