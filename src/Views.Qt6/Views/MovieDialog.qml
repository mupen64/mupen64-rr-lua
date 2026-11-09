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
    property string currentAuthor
    property string currentDescription
    property int currentStartFlags

    signal accepted()
    signal rejected()

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
                Layout.minimumHeight: 50
                wrapMode: TextArea.WordWrap

                text: root.currentAuthor
                onEditingFinished: root.currentAuthor = text
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
                Layout.minimumHeight: 50
                wrapMode: TextArea.WordWrap

                text: root.currentDescription
                onEditingFinished: root.currentDescription = text
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

                    clip: true
                    syncView: table
                }
                TableView {
                    id: table
                    Layout.fillHeight: true

                    flickableDirection: Flickable.HorizontalAndVerticalFlick
                    acceptedButtons: Qt.NoButton
                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }
                    ScrollBar.horizontal: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

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
                uniformCellSizes: true
                Layout.maximumWidth: 150
                GroupBox {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    //% "Start from"
                    title: qsTrId("dialogs.movie.startFrom")

                    ColumnLayout {
                        ButtonGroup {
                            id: groupStartFrom
                            onClicked: function (button: AbstractButton) {
                                root.currentStartFlags = button.value
                            }

                            checkedButton: {
                                const result = buttons.find(btn => btn.value == root.currentStartFlags);
                                return result ?? null;
                            }
                        }
                        RadioButton {
                            ButtonGroup.group: groupStartFrom
                            //% "Savestate"
                            text: qsTrId("dialogs.movie.startFrom.save")
                            enabled: !root.forPlayback

                            readonly property int value: VcrStartFrom.Snapshot
                        }
                        RadioButton {
                            ButtonGroup.group: groupStartFrom
                            //% "Reset"
                            text: qsTrId("dialogs.movie.startFrom.reset")
                            enabled: !root.forPlayback

                            readonly property int value: VcrStartFrom.Nothing
                        }
                        RadioButton {
                            ButtonGroup.group: groupStartFrom
                            //% "EEPROM"
                            text: qsTrId("dialogs.movie.startFrom.eeprom")
                            enabled: !root.forPlayback

                            readonly property int value: VcrStartFrom.Eeprom
                        }
                    }
                }
            }
        }
        DialogButtonBox {
            Layout.fillWidth: true
            alignment: Qt.AlignRight
            standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

            onAccepted: {
                root.close();
                root.accepted();
            }
            onRejected: {
                root.close();
                root.rejected();
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
            root.currentAuthor = author;
            root.currentDescription = description;
            root.currentStartFlags = startFlags;
        }
    }
}
