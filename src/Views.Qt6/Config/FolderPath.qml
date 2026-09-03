/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

import Utils

GroupBox {
    id: root

    Layout.fillWidth: true
    padding: 10

    property alias target: priv.dummy
    required target
    default required property Dialogs.FolderDialog dialog

    QtObject {
        id: priv
        property var dummy: null
    }

    RowLayout {
        anchors.fill: parent

        // onWidthChanged: console.log(`width: ${width}`)
        TextField {
            Layout.fillWidth: true
            readOnly: true
            text: root.target
        }
        Button {
            icon.name: "folder-open-symbolic"
            onClicked: root.dialog.open()
        }
    }

    onDialogChanged: {
        dialog.parentWindow = root.Window.window
    }

    Connections {
        enabled: root.dialog != null
        target: root.dialog
        function onAccepted() {
            root.target = Paths.toLocalFile(root.dialog.selectedFolder);
        }
    }
}
