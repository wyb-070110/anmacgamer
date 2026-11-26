import QtQuick
import QtQuick.Dialogs
import Anmacgamer.Controls
import Anmacgamer.App

SettingRow {
    id: row

    fact: Recordings.location

    Button {
        objectName: "chooseFolder"
        text: qsTr("Change…")
        onClicked: picker.open()
    }

    FolderDialog {
        id: picker

        title: row.title
        currentFolder: Recordings.locationUrl
        onAccepted: row.commit(Recordings.folderOf(picker.selectedFolder))
    }
}
