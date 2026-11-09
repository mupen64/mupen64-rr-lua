/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

import Mupen64.Core
import Mupen64.Components
import Mupen64.Utils

ApplicationWindow {
    id: root

    QtObject {
        id: priv

        //% "Record Movie"
        readonly property string recordTitle: qsTrId("dialogs.movie.recTitle")
        //% "Play Movie"
        readonly property string playTitle: qsTrId("dialogs.movie.playTitle")
    }
    function resetDialog() {
        currentPath = "";
    }

    property bool forPlayback
    property string currentPath

    modality: Qt.WindowModal
    // popupType: Dialog.Window
    // modal: true
    // standardButtons: Dialog.Ok | Dialog.Cancel
    title: forPlayback ? priv.playTitle : priv.recordTitle

    onVisibleChanged: {
        if (visible) resetDialog();
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 5
        GridLayout {
            columns: 2
            rowSpacing: 10
            columnSpacing: 10

            Label {
                Layout.fillWidth: true
                //% "Path:"
                text: qsTrId("dialogs.movie.path")
                horizontalAlignment: Text.AlignRight
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1
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
                Layout.fillWidth: true
                //% "Author:"
                text: qsTrId("dialogs.movie.author")
                horizontalAlignment: Text.AlignRight
            }
            TextArea {
                id: fieldAuthor
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1
                wrapMode: TextArea.WordWrap
            }

            Label {
                Layout.fillWidth: true
                //% "Description:"
                text: qsTrId("dialogs.movie.description")
                horizontalAlignment: Text.AlignRight
            }
            TextArea {
                id: fieldDescription
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1
                wrapMode: TextArea.WordWrap
            }
        }
        RowLayout {
            Item {
                id: tableRoot
                Layout.minimumHeight: 100
                Layout.fillHeight: true
                Layout.fillWidth: true
                HorizontalHeaderView {
                    id: tableHeader
                    anchors.top: tableRoot.top
                    anchors.left: tableRoot.left
                    anchors.right: tableRoot.right


                    syncView: table
                }
                TableView {
                    id: table
                    Layout.fillHeight: true

                    anchors.top: tableHeader.bottom
                    anchors.left: tableRoot.left
                    anchors.right: tableRoot.right
                    anchors.bottom: tableRoot.bottom

                    clip: true
                    model: metadata

                    delegate: SimpleTableCell {}
                }
            }
            ColumnLayout {
                GroupBox {
                    title: "Start from"
                }
            }
        }
    }

    // Auxiliary objects
    // ===================
    Dialogs.FileDialog {
        id: fileDialog

        title: root.title
        fileMode: root.forPlayback ? Dialogs.FileDialog.OpenFile : Dialogs.FileDialog.SaveFile
        //% "Mupen64 Movie"
        nameFilters: [`${qsTrId("formats.m64")} (*.m64 *.rec)`]

        onAccepted: {
            root.currentPath = Paths.toLocalFile(selectedFile)
            metadata.loadMovie(root.currentPath);
        }
    }

    MovieMetadata {
        id: metadata
        onValuesUpdated: {
            fieldAuthor.text = author;
            fieldDescription.text = description;
        }
    }
}
