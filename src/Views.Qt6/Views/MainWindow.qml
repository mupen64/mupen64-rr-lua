/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as Dialogs

import Core

ApplicationWindow {
    id: mainWindow
    width: 640
    height: 480
    visible: true

    title: qsTr("Mupen64RR")

    // onClosing: (close) => {
    //     core.vrCloseROM();
    // }

    // CONTENT
    // =====================================

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

    Button {
        anchors.centerIn: parent

        text: "foo the bar"
        onClicked: {
            mainWindow.queueInfoDialog(() => {
                console.log("done info dialog");
            }, "dialog 1", "yay, dialog!", CoreDialogType.Information)
            mainWindow.queueAskDialog((result) => {
                console.log(`done ask dialog: ${result}`);
            }, "dialog 2", "yay, more dialog!", CoreDialogType.Warning)
        }
    }

    // AUXILIARY OBJECTS
    // =====================================

    CoreContext {
        id: core

        onOpenInfoDialog: mainWindow.queueInfoDialog
        onOpenAskDialog: mainWindow.queueAskDialog
        onOpenMultiDialog: mainWindow.queueMultiDialog
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
    // TODO: can this be moved somewhere else?

    property list<var> dialogQueue: []
    property var currDialog: null

    function showNextDialog() {
        // no more dialogs left; clear currDialog
        if (dialogQueue.length === 0) {
            currDialog = null;
            return;
        }

        // grab the next dialog out of the queue and open it
        currDialog = dialogQueue.shift();
        switch (currDialog.type) {
            case "info":
                diaServiceInfo.title = currDialog.title;
                diaServiceInfo.content = currDialog.content;
                diaServiceInfo.coreType = currDialog.coreType;
                diaServiceInfo.open()
                break;
            case "ask":
                diaServiceAsk.title = currDialog.title;
                diaServiceAsk.content = currDialog.content;
                diaServiceAsk.coreType = currDialog.coreType;
                diaServiceAsk.open()
                break;
            case "multi":
                diaServiceAsk.title = currDialog.title;
                diaServiceAsk.content = currDialog.content;
                diaServiceAsk.coreType = currDialog.coreType;
                diaServiceAsk.choices = currDialog.choices;
                diaServiceAsk.open()
                break;
        }
    }

    function queueInfoDialog(done, title, content, type) {
        // queue the current dialog
        dialogQueue.push({
            type: "info",
            done: done,
            title: title,
            content: content,
            coreType: type,
        });
        // start the queue if needed
        if (currDialog == null)
            showNextDialog();
    }

    function queueAskDialog(done, title, content, type) {
        dialogQueue.push({
            type: "ask",
            done: done,
            title: title,
            content: content,
            coreType: type,
        });
        if (currDialog == null)
            showNextDialog();
    }

    function queueMultiDialog(done, title, content, choices, type) {
        dialogQueue.push({
            type: "multi",
            done: done,
            title: title,
            content: content,
            choices: choices,
            coreType: type,
        });
        if (currDialog == null)
            showNextDialog();
    }



    MessageBox {
        id: diaServiceInfo
        standardButtons: Dialog.Ok
        onAccepted: {
            if (mainWindow.currDialog != null)
                mainWindow.currDialog.done();
            mainWindow.showNextDialog();
        }
    }

    MessageBox {
        id: diaServiceAsk
        standardButtons: Dialog.Yes | Dialog.No

        onAccepted: {
            if (mainWindow.currDialog != null)
                mainWindow.currDialog.done(true);
            mainWindow.showNextDialog();
        }
        onRejected: {
            if (mainWindow.currDialog != null)
                mainWindow.currDialog.done(false);
            mainWindow.showNextDialog();
        }
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

        onAccepted: {
            if (mainWindow.currDialog != null)
                mainWindow.currDialog.done(lastSelected);
            mainWindow.showNextDialog();
        }
    }
}