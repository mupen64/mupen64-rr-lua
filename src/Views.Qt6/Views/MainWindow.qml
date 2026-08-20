/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
        onClicked: mainWindow.showDialog("dialog!", "yay, a dialog!", CoreDialogType.Information)
    }

    // AUXILIARY OBJECTS
    // =====================================

    CoreContext {
        id: core
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

    Dialog {
        id: diaMessage
        modal: true
        popupType: Popup.Window
        standardButtons: Dialog.Ok | Dialog.Cancel

        // Lock the minimum and maximum bounds of the underlying window

        property int coreType
        property string content

        contentItem: RowLayout {
            Window.onWindowChanged: {
                // Lock the created window's width/height when it is displayed.
                if (Window.window !== null && Window.window !== mainWindow) {
                    Window.window.width = width
                    Window.window.height = height

                    Window.window.minimumWidth = Window.window.width
                    Window.window.maximumWidth = Window.window.width
                    Window.window.minimumHeight = Window.window.height
                    Window.window.maximumHeight = Window.window.height
                }
            }

            spacing: 16
            Item {
                Layout.minimumWidth: 32
                Layout.fillHeight: true

                Image {
                    anchors.centerIn: parent
                    width: 32
                    height: 32

                    source: (() => {
                        switch (diaMessage.coreType) {
                            case CoreDialogType.Error:
                                return "image://icons/dialog-error";
                            case CoreDialogType.Warning:
                                return "image://icons/dialog-warning";
                            case CoreDialogType.Information:
                            default:
                                return "image://icons/dialog-information";
                        }
                    })()
                }
            }
            Label {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 256
                
                verticalAlignment: Text.AlignVCenter

                text: diaMessage.content
            }
        }
    }

    // DIALOG SERVICE
    // =====================================

    function showDialog(title, content, type) {
        diaMessage.title = title;
        diaMessage.content = content;
        diaMessage.coreType = type;

        diaMessage.open()
    }
}