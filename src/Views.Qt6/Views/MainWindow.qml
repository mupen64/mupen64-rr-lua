/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

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
        onClicked: mainWindow.showMultipleChoiceDialog("dialog!", "yay, a dialog!", ["really?", "no way", "be fr rn"], CoreDialogType.Error)
    }

    // AUXILIARY OBJECTS
    // =====================================

    CoreContext {
        id: core

        onShowDialog: showDialog
        onShowAskDialog: showAskDialog
        onShowMultipleChoiceDialog: showMultipleChoiceDialog
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
        id: diaService
        standardButtons: Dialog.Ok

        onAccepted: core.showDialogFinished()
    }
    function showDialog(title, content, type) {
        diaService.title = title;
        diaService.content = content;
        diaService.coreType = type;

        diaService.open();
    }

    MessageBox {
        id: diaServiceAsk
        standardButtons: Dialog.Yes | Dialog.No

        onAccepted: core.showAskDialogFinished(true)
        onRejected: core.showAskDialogFinished(false)
    }
    function showAskDialog(title, content, type) {
        diaServiceAsk.title = title;
        diaServiceAsk.content = content;
        diaServiceAsk.coreType = type;

        diaServiceAsk.open();
    }

    MessageBox {
        id: diaServiceMulti
        property list<string> choices
        property int lastSelected

        footer: DialogButtonBox {
            Repeater {
                model: diaServiceMulti.choices
                Button {
                    required property int index
                    required property string modelData

                    DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                    text: modelData
                    onClicked: diaServiceMulti.lastSelected = index
                }
            }
        }

        onAccepted: core.showMultipleChoiceDialogFinished(lastSelected)
    }
    function showMultipleChoiceDialog(title, content, choices, type) {
        diaServiceMulti.title = title;
        diaServiceMulti.content = content;
        diaServiceMulti.choices = choices;
        diaServiceMulti.coreType = type;

        diaServiceMulti.open();
    }
}