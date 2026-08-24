/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts

import Core

ApplicationWindow {
    id: mainWindow
    visible: true
    title: qsTr("Mupen64RR")

    property bool lockSize;

    // ensure window fits content
    minimumWidth: mainStack.implicitWidth + leftPadding + rightPadding
    minimumHeight: mainStack.implicitHeight + topPadding + bottomPadding

    Binding {
        when: EmuContext.emuLaunched && mainWindow.minimumWidth > 0 && mainWindow.minimumHeight > 0
        mainWindow.maximumWidth: mainWindow.minimumWidth
        mainWindow.maximumHeight: mainWindow.minimumHeight
    }

    // CONTENT
    // =====================================

    header: MenuBar {
        Menu {
            title: qsTr("File")
            Action {
                text: qsTr("Load ROM...")
                onTriggered: diaOpenRom.open()
            }
            Action {
                text: qsTr("Close ROM")
                onTriggered: EmuContext.vrCloseROM()
            }
            Action {
                text: qsTr("Reset ROM")
                onTriggered: EmuContext.vrResetROM()
            }
        }
        Menu {
            title: qsTr("Emulation")
            Action {
                text: qsTr("Pause")
            }
            Action {
                text: qsTr("Speed Down")
            }
            Action {
                text: qsTr("Speed Up")
            }
            Action {
                text: qsTr("Reset Speed")
            }
            Action {
                text: qsTr("GS Button")
            }
            MenuSeparator {}
            Action {
                text: qsTr("Frame Advance")
            }
            // TODO: multi-frame advance
            MenuSeparator {}
            Menu {
                title: qsTr("Save State")
                Action {
                    text: qsTr("Save Current Slot")
                }
                Action {
                    text: qsTr("Save as File...")
                }
            }
            MenuSeparator {}
            Menu {
                title: qsTr("Load State")
                Action {
                    text: qsTr("Load Current Slot")
                }
                Action {
                    text: qsTr("Load from File...")
                }
            }
            MenuSeparator {}
            Menu {
                title: qsTr("Current State Slot")
            }
        }
        Menu {
            title: qsTr("Options")
        }
    }

    StackLayout {
        id: mainStack
        anchors.fill: parent

        currentIndex: (EmuContext.emuLaunched) ? 1 : 0

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Button {
                anchors.centerIn: parent

                text: "foo the bar"
            }
        }
        Item {
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height

            EmuDisplay {
                id: coreDisplay
                context: EmuContext
                anchors.top: parent.top
                anchors.left: parent.left
            }
        }
    }

    // AUXILIARY OBJECTS
    // =====================================

    Dialogs.FileDialog {
        id: diaOpenRom
        title: qsTr("Open ROM...")
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: [`${qsTr("N64 ROMs")} (*.n64 *.z64 *.v64)`]
        onAccepted: {
            EmuContext.vrStartROM(selectedFile);
        }
    }

    // update the video output when the emulator is running
    FrameAnimation {
        running: EmuContext.emuLaunched
        onTriggered: EmuContext.vrInvalidateVisuals()
    }

    DialogService {
        id: dialogService
    }
}
