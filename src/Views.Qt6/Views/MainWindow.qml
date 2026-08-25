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

    // ensure window fits content
    minimumWidth: mainStack.implicitWidth + leftPadding + rightPadding
    minimumHeight: mainStack.implicitHeight + topPadding + bottomPadding

    // CONTENT
    // =====================================

    header: MenuBar {
        id: menuBar
        property bool opened: false

        delegate: MenuBarItem {
            id: tempItem
            // update menuBar.opened as needed
            Connections {
                target: tempItem.menu
                enabled: tempItem.menu != null
                function onAboutToShow() {
                    tempItem.updateOpened()
                }
                function onClosed() {
                    tempItem.updateOpened()
                }
            }
            function updateOpened() {
                menuBar.opened = menuBar.menus.some(child => child.visible);
            }
        }

        Menu {
            title: qsTr("File")
            Action {
                id: actLoadROM
                text: qsTr("Load ROM...")
                onTriggered: diaOpenRom.open()
            }
            Action {
                id: actCloseROM
                text: qsTr("Close ROM")
                enabled: core.emuLaunched
                onTriggered: core.vrCloseROM()
            }
            Action {
                text: qsTr("Reset ROM")
                enabled: core.emuLaunched
                onTriggered: core.vrResetROM()
            }
        }
        Menu {
            title: qsTr("Emulation")
            Action {
                id: actPause
                text: qsTr("Pause")
                enabled: core.emuLaunched
                checkable: true
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
                MenuSeparator {}
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
                MenuSeparator {}
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

        currentIndex: (core.emuLaunched) ? 1 : 0

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
                context: core
                anchors.top: parent.top
                anchors.left: parent.left
            }
        }
    }

    // Core context
    // =====================================

    EmuContext {
        id: core

        onEmuLaunchedChanged: {
            actPause.checked = false;
        }

        // pause when menu open
        emuPaused: menuBar.opened || actPause.checked

        // Graphics integration
        onGfxRequestSize: (width, height) => coreDisplay.reserveSize(width, height)
        onUpdateScreen: coreDisplay.readPixels()

        // Dialog service
        onOpenInfoDialog: dialogService.queueInfoDialog
        onOpenAskDialog: dialogService.queueAskDialog
        onOpenMultiDialog: dialogService.queueMultiDialog
    }

    // update the video output when the emulator is running
    FrameAnimation {
        running: core.emuLaunched
        onTriggered: core.vrInvalidateVisuals()
    }

    // lock the window size when the emulator is running
    Binding {
        when: core.emuLaunched && mainWindow.minimumWidth > 0 && mainWindow.minimumHeight > 0
        mainWindow.maximumWidth: mainWindow.minimumWidth
        mainWindow.maximumHeight: mainWindow.minimumHeight
    }

    // AUXILIARY OBJECTS
    // =====================================

    Dialogs.FileDialog {
        id: diaOpenRom
        title: qsTr("Open ROM...")
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: [`${qsTr("N64 ROMs")} (*.n64 *.z64 *.v64)`]
        onAccepted: {
            core.vrStartROM(selectedFile);
        }
    }

    DialogService {
        id: dialogService
    }
}
