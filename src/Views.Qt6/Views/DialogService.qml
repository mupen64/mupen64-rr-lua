/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

import Components

Item {
    id: root
    signal dialogFinished(int id, int result)

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
                diaServiceMulti.lastSelected = -1;
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
            if (currDialog !== null && currDialog.id >= 0)
                root.dialogFinished(currDialog.id, result === undefined ? 0 : Number(result));
            showNextDialog();
        }
    }

    function discardDialogs(): void {
        priv.dialogQueue = [];
        priv.currDialog = null;
        diaServiceInfo.close();
        diaServiceAsk.close();
        diaServiceMulti.close();
    }

    function queueInfoDialog(id: int, title: string, content: string, type: int): void {
        priv.queueDialogInner({
            type: "info",
            id: id,
            title: title,
            content: content,
            coreType: type
        });
    }

    function queueAskDialog(id: int, title: string, content: string, type: int): void {
        priv.queueDialogInner({
            type: "ask",
            id: id,
            title: title,
            content: content,
            coreType: type
        });
    }

    function queueMultiDialog(id: int, title: string, content: string, choices: list<string>, type: int): void {
        priv.queueDialogInner({
            type: "multi",
            id: id,
            title: title,
            content: content,
            choices: choices,
            coreType: type
        });
    }

    MessageBox {
        id: diaServiceInfo
        standardButtons: Dialog.Ok
        onClosed: priv.dialogClosed()
    }

    MessageBox {
        id: diaServiceAsk
        standardButtons: Dialog.Yes | Dialog.No

        onClosed: priv.dialogClosed(result == Dialog.Accepted)
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

        onClosed: priv.dialogClosed(result === Dialog.Accepted ? diaServiceMulti.lastSelected : -1)
    }
}
