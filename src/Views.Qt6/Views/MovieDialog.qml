/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

import Mupen64.Components
import Mupen64.Utils

DialogBase {
    id: root

    QtObject {
        id: priv

        //% "Record Movie"
        readonly property string recordTitle: qsTrId("dialogs.movie.recTitle")
        //% "Play Movie"
        readonly property string playTitle: qsTrId("dialogs.movie.playTitle")
    }

    property bool forPlayback
    property string currentPath

    title: forPlayback ? priv.playTitle : priv.recordTitle

    onOpened: {
        // reset dialog state
        currentPath = "";
    }

    ColumnLayout {
        GridLayout {
            columns: 2
            rowSpacing: 10
            columnSpacing: 10

            Label {
                Layout.maximumWidth: 50
                //% "Path:"
                text: qsTrId("dialogs.movie.path")
                horizontalAlignment: Text.AlignRight
            }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    Layout.fillWidth: true
                    readOnly: true
                    text: root.currentPath
                }
                Button {
                    icon.name: "folder-open-symbolic"
                    onClicked: fileDialog.open()
                }
            }

            Label {
                Layout.maximumWidth: 50
                //% "Author:"
                text: qsTrId("dialogs.movie.author")
                horizontalAlignment: Text.AlignRight
            }
            TextArea {
                Layout.fillWidth: true
                wrapMode: TextArea.WordWrap
            }

            Label {
                Layout.maximumWidth: 50
                //% "Description:"
                text: qsTrId("dialogs.movie.description")
                horizontalAlignment: Text.AlignRight
            }
            TextArea {
                Layout.fillWidth: true
                wrapMode: TextArea.WordWrap
            }
        }
    }

    // Dialogs
    // ===================
    Dialogs.FileDialog {
        id: fileDialog

        title: root.title
        fileMode: root.forPlayback ? Dialogs.FileDialog.OpenFile : Dialogs.FileDialog.SaveFile
        //% "Mupen64 Movie"
        nameFilters: [`${qsTrId("formats.m64")} (*.m64 *.rec)`]

        onAccepted: {
            root.currentPath = Paths.toLocalFile(selectedFile)
        }
    }
}
