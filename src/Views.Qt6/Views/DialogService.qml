/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

// import Core

Item {
    QtObject {
        id: priv
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
                diaServiceInfo.open();
                break;
            case "ask":
                diaServiceAsk.title = currDialog.title;
                diaServiceAsk.content = currDialog.content;
                diaServiceAsk.coreType = currDialog.coreType;
                diaServiceAsk.open();
                break;
            case "multi":
                diaServiceMulti.title = currDialog.title;
                diaServiceMulti.content = currDialog.content;
                diaServiceMulti.coreType = currDialog.coreType;
                diaServiceMulti.choices = currDialog.choices;
                diaServiceMulti.open();
                break;
            }
        }
        function queueDialogInner(obj) {
            dialogQueue.push(obj)
            if (currDialog === null)
                showNextDialog()
        }
        function dialogClosed(result = undefined) {
            if (result !== undefined && currDialog.done != null)
                currDialog.done(result);
            showNextDialog();
        }
    }

    function queueInfoDialog(done, title, content, type) {
        priv.queueDialogInner({
            type: "info",
            done: done,
            title: title,
            content: content,
            coreType: type
        });
    }

    function queueAskDialog(done, title, content, type) {
        priv.queueDialogInner({
            type: "ask",
            done: done,
            title: title,
            content: content,
            coreType: type
        });
    }

    function queueMultiDialog(done, title, content, choices, type) {
        priv.queueDialogInner({
            type: "multi",
            done: done,
            title: title,
            content: content,
            choices: choices,
            coreType: type
        });
    }

    MessageBox {
        id: diaServiceInfo
        standardButtons: Dialog.Ok
        onAccepted: priv.dialogClosed()
    }

    MessageBox {
        id: diaServiceAsk
        standardButtons: Dialog.Yes | Dialog.No

        onAccepted: priv.dialogClosed(true)
        onRejected: priv.dialogClosed(false)
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

        onAccepted: priv.dialogClosed(lastSelected)
    }
}
