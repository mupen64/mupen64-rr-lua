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
    padding: 10

    property string path

    signal openDialog()
    signal pathModified()

    function setPath(newPath: string) {
        if (newPath != path) {
            path = newPath;
            pathModified();
        }
    }

    RowLayout {
        anchors.fill: parent

        TextField {
            Layout.fillWidth: true
            readOnly: true
            text: root.path
        }
        Button {
            icon.name: "folder-open-symbolic"
            onClicked: root.openDialog()
        }
    }
}
