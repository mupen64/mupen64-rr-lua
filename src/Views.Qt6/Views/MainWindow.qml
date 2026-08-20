/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts

import Core

ApplicationWindow {
    id: mainWindow
    width: 640
    height: 480
    visible: true

    title: qsTr("Mupen64RR")

    header: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Open ROM...")
                onTriggered: diaOpenRom.open()
            }
            Action {
                text: qsTr("&Close ROM...")
                onTriggered: core.vrCloseROM()
                
            }
            MenuSeparator { }
            Action {
                text: qsTr("&Exit")
                onTriggered: Qt.quit()
            }
        }
    }

    onClosing: (close) => {
        core.vrCloseROM();
    }

    // CONTENT
    // =====================================

    Button {
        anchors.centerIn: parent

        text: "foo the bar"
        onClicked: mainWindow.showDialog("dialog!", "yay, a dialog!", CoreDialogType.Information)
    }

    // AUXILIARY OBJECTS
    // =====================================

    CoreContext {
        id: core
    }

    Dialogs.FileDialog {
        id: diaOpenRom
        title: qsTr("Open ROM...")
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: [`${qsTr("N64 ROMs")} (*.n64 *.z64 *.v64)`]
        onAccepted: {
            core.vrStartROM(selectedFile);
        }
    }

    // DIALOG SERVICE
    // =====================================

    MessageBox {
        id: diaMessage
        standardButtons: Dialog.Ok | Dialog.Cancel
    }

    function showDialog(title, content, type) {
        console.log(`core.showDialog: ${core.showDialog}`)

        diaMessage.title = title;
        diaMessage.content = content;
        diaMessage.coreType = type;

        diaMessage.open()
    }
}