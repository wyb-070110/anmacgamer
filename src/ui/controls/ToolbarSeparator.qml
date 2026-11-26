import QtQuick
import Anmacgamer.Theme

Item {
    id: separator

    // Lying across a toolbar whose buttons stand in a column (FloatingToolbar.vertical).
    readonly property bool across: {
        return parent instanceof Grid ? parent.columns === 1 : parent instanceof Column
    }

    implicitWidth: across ? Theme.size.toolbarButton : Theme.size.stroke + 2 * Theme.space.xs
    implicitHeight: across ? Theme.size.stroke + 2 * Theme.space.xs : Theme.size.toolbarButton

    Accessible.role: Accessible.Separator

    Rectangle {
        anchors.centerIn: parent
        width: separator.across ? Theme.size.toolbarSeparator : Theme.size.stroke
        height: separator.across ? Theme.size.stroke : Theme.size.toolbarSeparator
        color: Theme.colors.strokeDefault
    }
}
