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

    // MENU BAR
    // =====================================

    header: MenuBar {
        id: menuBar
        // binding is not implemented here as it won't update properly
        property bool opened: false

        delegate: MenuBarItem {
            id: item
            // update menuBar.opened as needed
            Connections {
                target: item.menu
                enabled: item.menu != null
                function onAboutToShow() {
                    item.updateOpened()
                }
                function onClosed() {
                    item.updateOpened()
                }
            }
            function updateOpened() {
                menuBar.opened = menuBar.menus.some(child => child.visible);
            }
        }

        Menu {
            title: qsTr("File")
            Action {
                text: qsTr("Load ROM...")
                onTriggered: diaOpenRom.open()
            }
            Action {
                text: qsTr("Close ROM")
                enabled: core.launched
                onTriggered: core.closeROM()
            }
            Action {
                text: qsTr("Reset ROM")
                enabled: core.launched
                onTriggered: core.resetROM()
            }
        }
        Menu {
            title: qsTr("Emulation")
            Action {
                id: actPause
                checkable: true
                text: qsTr("Pause")
                enabled: core.launched
            }
            Action {
                text: qsTr("Speed Down")
                enabled: core.launched
                onTriggered: core.speedModifier -= 5
            }
            Action {
                text: qsTr("Speed Up")
                enabled: core.launched
                onTriggered: core.speedModifier += 5
            }
            Action {
                text: qsTr("Reset Speed")
                enabled: core.launched
                onTriggered: core.speedModifier = 100
            }
            Action {
                id: actGSButton
                enabled: core.launched
                text: qsTr("GS Button")
                checkable: true
                
            }
            MenuSeparator {}
            Action {
                text: qsTr("Frame Advance")
                enabled: core.launched
                onTriggered: {
                    actPause.checked = true;
                    core.frameAdvance(1);
                }
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

    // Bindings and signals to attach for menu items
    Binding {
        core.paused: menuBar.opened || actPause.checked
        core.gsButton: actGSButton.checked
    }
    Connections {
        target: core
        function onLaunchedChanged() {
            actPause.checked = false
            actGSButton.checked = false
        }
    }

    // CONTENT VIEW
    // =====================================

    StackLayout {
        id: mainStack
        anchors.fill: parent

        currentIndex: (core.launched) ? 1 : 0

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
        running: core.launched
        onTriggered: core.invalidateVisuals()
    }

    // lock the window size when the emulator is running
    Binding {
        when: core.launched && mainWindow.minimumWidth > 0 && mainWindow.minimumHeight > 0
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
            core.startROM(selectedFile);
        }
    }

    DialogService {
        id: dialogService
    }
}
