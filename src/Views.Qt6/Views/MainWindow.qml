import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs 

import Core

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
                onTriggered: MupenCore.vrCloseROM()
                
            }
            MenuSeparator { }
            Action {
                text: qsTr("&Exit")
                onTriggered: Qt.quit()
            }
        }
    }

    onClosing: function(close) {
        MupenCore.vrCloseROM();
    }

    FileDialog {
        id: fdOpenRom
        title: qsTr("Open ROM...")
        fileMode: FileDialog.OpenFile
        nameFilters: [`${qsTr("N64 ROMs")} (*.n64 *.z64 *.v64)`]
        onAccepted: {
            MupenCore.vrStartROM(selectedFile);
        }
    }
}