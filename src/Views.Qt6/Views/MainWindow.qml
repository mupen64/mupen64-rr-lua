/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
                onTriggered: core.vrCloseROM()
                
            }
            MenuSeparator { }
            Action {
                text: qsTr("&Exit")
                onTriggered: Qt.quit()
            }
        }
    }

    onClosing: function(close) {
        core.vrCloseROM();
    }

    CoreContext {
        id: core
    }

    FileDialog {
        id: fdOpenRom
        title: qsTr("Open ROM...")
        fileMode: FileDialog.OpenFile
        nameFilters: [`${qsTr("N64 ROMs")} (*.n64 *.z64 *.v64)`]
        onAccepted: {
            core.vrStartROM(selectedFile);
        }
    }
}