import QtQuick
import QtQuick.Effects
import Anmacgamer.Theme

Item {
    id: toolbar

    default property alias content: row.data
    // Buttons in a column instead of a row, for a toolbar on the left or right of the picture.
    property bool vertical: false
    // Where tooltips and menus of the buttons open: away from the edge the toolbar sits on.
    property string popupEdge: "bottom"
    property bool pinned: false
    property bool blocked: false
    property bool flat: false
    property int hideDelay: Theme.motion.toolbarHideDelay
    readonly property bool shown: internal.revealed
    readonly property bool hovered: hover.hovered

    function reveal() {
        if (blocked)
            return
        internal.revealed = true
        internal.schedule()
    }

    function conceal() {
        if (pinned || hovered)
            return
        idle.stop()
        internal.revealed = false
    }

    implicitWidth: vertical ? Math.max(Theme.size.toolbar, row.implicitWidth + 2 * Theme.space.xs)
                            : row.implicitWidth + 2 * Theme.space.xs
    implicitHeight: vertical ? row.implicitHeight + 2 * Theme.space.xs : Theme.size.toolbar
    opacity: internal.revealed ? 1 : 0
    visible: opacity > 0

    Accessible.role: Accessible.ToolBar

    onPinnedChanged: internal.schedule()
    onHideDelayChanged: internal.schedule()
    onVerticalChanged: internal.syncGrid()
    Component.onCompleted: internal.syncGrid()
    onHoveredChanged: {
        if (hovered && visible && !blocked)
            internal.revealed = true
        internal.schedule()
    }

    Behavior on opacity {
        id: fade

        NumberAnimation {
            duration: fade.targetValue > 0 ? Theme.motion.fast : Theme.motion.base
            easing.type: Easing.BezierSpline
            easing.bezierCurve: fade.targetValue > 0 ? Theme.motion.standard : Theme.motion.exit
        }
    }

    QtObject {
        id: internal

        property bool revealed: false

        function schedule() {
            if (revealed && !toolbar.pinned && !toolbar.hovered)
                idle.restart()
            else
                idle.stop()
        }

        function syncGrid() {
            if (toolbar.vertical) {
                row.rows = -1
                row.columns = 1
            } else {
                row.columns = -1
                row.rows = 1
            }
        }
    }

    Timer {
        id: idle

        interval: toolbar.hideDelay
        onTriggered: toolbar.conceal()
    }

    HoverHandler {
        id: hover
    }

    Item {
        anchors.fill: parent
        transform: Translate {
            x: toolbar.vertical ? (toolbar.opacity - 1) * (Theme.space.xs + Theme.space.xxs) : 0
            y: toolbar.vertical ? 0 : (toolbar.opacity - 1) * (Theme.space.xs + Theme.space.xxs)
        }

        RectangularShadow {
            anchors.fill: face
            visible: !toolbar.flat
            radius: face.radius
            offset.y: Theme.shadow.keyOffset
            blur: Theme.shadow.keyBlur
            color: Theme.shadow.key
        }

        Rectangle {
            id: face

            objectName: "face"
            anchors.fill: parent
            visible: !toolbar.flat
            radius: Math.min(width, height) / 2
            color: Theme.reducedTransparency ? Theme.colors.bgElevated : Theme.colors.bgFrosted
            border.width: Theme.size.stroke
            border.color: Theme.liquid ? Theme.colors.strokeDefault : Theme.colors.strokeStrong

            // Liquid Glass sheen and lit top edge, as on PopupSurface.
            Rectangle {
                anchors.fill: parent
                anchors.margins: Theme.size.stroke
                radius: Math.max(0, face.radius - Theme.size.stroke)
                visible: Theme.liquid
                gradient: Gradient {
                    GradientStop { position: 0; color: Theme.dark ? "#1AFFFFFF" : "#73FFFFFF" }
                    GradientStop { position: 0.5; color: "#00FFFFFF" }
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

        // One row of buttons, or one column while the toolbar is vertical. The buttons stay in this one grid:
        // moving them between a Row and a Column left the ones that were hidden at the time undrawn once shown.
        Grid {
            id: row

            objectName: "toolbarButtons"
            anchors.centerIn: parent
            // -1: as many as the visible buttons need. syncGrid() opens the unbounded axis before closing the
            // other so switching sides never passes through a 1x1 grid warning.
            columns: -1
            rows: 1
            rowSpacing: Theme.space.xxs
            columnSpacing: Theme.space.xxs
            horizontalItemAlignment: Grid.AlignHCenter
            verticalItemAlignment: Grid.AlignVCenter
        }
    }
}
