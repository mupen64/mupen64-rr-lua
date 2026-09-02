/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

GroupBox {
    id: root

    padding: 10

    property alias target: priv.dummy
    required target
    default required property Dialogs.FileDialog dialog

    QtObject {
        id: priv
        property var dummy: null
    }

    RowLayout {
        anchors.fill: parent
        TextField {
            Layout.fillWidth: true
            readOnly: true
            text: root.target
        }
        Button {
            icon.name: "folder-open"
            onClicked: root.dialog.open()
        }
    }

    onDialogChanged: {
        dialog.parent = root
    }

    Connections {
        enabled: root.dialog != null
        function onAccepted() {
            root.target = root.dialog.selectedFile
        }
    }
}
