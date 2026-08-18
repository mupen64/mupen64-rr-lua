import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs 

ApplicationWindow {
    width: 640
    height: 480
    visible: true

    title: qsTr("Mupen64RR")

    header: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Open ROM...")
                onTriggered: fdOpenRom.open()
            }
            Action {
                text: qsTr("&Close ROM...")
                
            }
            MenuSeparator { }
            Action {
                text: qsTr("&Exit")
                onTriggered: Qt.quit()
            }
        }
    }

    FileDialog {
        id: fdOpenRom
        title: qsTr("Open ROM...")
        fileMode: FileDialog.OpenFile
        nameFilters: [`${qsTr("N64 ROMs")} (*.n64 *.z64 *.v64)`]
    }
}