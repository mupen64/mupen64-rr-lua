/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as Dialogs

import Core

MenuBar {
    id: root
    required property EmuContext core
    required property DialogService dialogService

    // True if the menu has been opened in any capacity.
    readonly property bool opened: priv.opened

    QtObject {
        id: priv
        property bool opened: false
        function findBaseItemIndex(menu, target) {
            return menu.contentChildren.findIndex(item => item == target) + 1
        }
        function showDialogForError(result) {
            let message = CoreResult.message(result);
            if (message == null)
                return;

            let title = `${message.module} Error ${result}`;
            root.dialogService.queueInfoDialog(null, title, message.error, CoreDialogType.Error);
        }
    }

    // FIXME (MacOS): May not work as intended with native menus.
    delegate: MenuBarItem {
        id: item
        Connections {
            target: item.menu
            enabled: item.menu != null
            function onAboutToShow() {
                item.updateOpened();
            }
            function onClosed() {
                item.updateOpened();
            }
        }
        function updateOpened() {
            priv.opened = root.menus.some(child => child.visible);
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
            enabled: root.core.launched
            onTriggered: {
                let result = root.core.closeROM();
                priv.showDialogForError(result);
            }
        }
        Action {
            text: qsTr("Reset ROM")
            enabled: root.core.launched
            onTriggered: {
                let result = root.core.resetROM();
                priv.showDialogForError(result);
            }
        }
    }
    Menu {
        title: qsTr("Emulation")
        Action {
            id: actPause
            checkable: true
            text: qsTr("Pause")
            enabled: root.core.launched
        }
        Action {
            text: qsTr("Speed Down")
            enabled: root.core.launched
            onTriggered: root.core.speedModifier -= 5
        }
        Action {
            text: qsTr("Speed Up")
            enabled: root.core.launched
            onTriggered: root.core.speedModifier += 5
        }
        Action {
            text: qsTr("Reset Speed")
            enabled: root.core.launched
            onTriggered: root.core.speedModifier = 100
        }
        Action {
            id: actGSButton
            enabled: root.core.launched
            text: qsTr("GS Button")
            checkable: true
        }
        MenuSeparator {}
        Action {
            text: qsTr("Frame Advance")
            enabled: root.core.launched
            onTriggered: {
                actPause.checked = true;
                root.core.frameAdvance(1);
            }
        }
        Action {
            id: actMultiFrameAdvance
            property int frameCount: 0

            text: qsTr("Multi-Frame Advance")
            enabled: root.core.launched
            onTriggered: {
                if (frameCount == 0) return;
                actPause.checked = true;
                root.core.frameAdvance(frameCount);
            }
        }
        Action {
            text: qsTr("Multi-Frame Advance +1")
            enabled: root.core.launched
            onTriggered: {
                // TODO: should this be capped?
                actMultiFrameAdvance.frameCount += 1;
            }
        }
        Action {
            text: qsTr("Multi-Frame Advance -1")
            enabled: root.core.launched
            onTriggered: {
                if (actMultiFrameAdvance.frameCount > 0)
                    actMultiFrameAdvance.frameCount -= 1;
            }
        }
        Action {
            text: qsTr("Multi-Frame Advance Reset")
            enabled: root.core.launched
            onTriggered: {
                // TODO: supply this from config
                actMultiFrameAdvance.frameCount = 0;
            }
        }
        Menu {
            id: menuSaveState
            title: qsTr("Save State")
            Action {
                text: qsTr("Save Current Slot")
                enabled: root.core.launched
            }
            Action {
                text: qsTr("Save as File...")
                enabled: root.core.launched
                onTriggered: diaSaveState.open()
            }
            MenuSeparator {
                id: sepSaveSlots
            }
            Instantiator {
                model: 10

                delegate: Action {
                    required property int index
                    text: `Save Slot ${index + 1}`
                }

                onObjectAdded: (index, object) => {
                    // ensure object is added to correct position relative to separator
                    let baseIndex = priv.findBaseItemIndex(menuSaveState, sepSaveSlots);
                    menuSaveState.insertAction(baseIndex + index, object);
                }
                onObjectRemoved: (index, object) => menuSaveState.removeAction(object)
            }
        }
        Menu {
            id: menuLoadState
            title: qsTr("Load State")
            Action {
                text: qsTr("Load Current Slot")
                enabled: root.core.launched
                onTriggered: {
                    let currSlot = menuCurrSlot.selectedIndex;
                    root.core.saveSlot(currSlot);
                }
            }
            Action {
                text: qsTr("Load from File...")
                enabled: root.core.launched
                onTriggered: diaLoadState.open()
            }
            MenuSeparator {
                id: sepLoadSlots
            }
            Instantiator {
                model: 10

                delegate: Action {
                    required property int index
                    text: `Load Slot ${index + 1}`
                }

                onObjectAdded: (index, object) => {
                    // ensure object is added to correct position relative to separator
                    let baseIndex = priv.findBaseItemIndex(menuLoadState, sepLoadSlots);
                    menuLoadState.insertAction(baseIndex + index, object);
                }
                onObjectRemoved: (index, object) => menuLoadState.removeAction(object)
            }
        }
        MenuSeparator {}
        Menu {
            id: menuCurrSlot
            title: qsTr("Current State Slot")

            // convenience property for checking the current slot
            property int selectedIndex: groupCurrentSlot.checkedAction.index

            ActionGroup {
                id: groupCurrentSlot
                exclusive: true
            }
            Instantiator {
                model: 10

                delegate: Action {
                    required property int index
                    ActionGroup.group: groupCurrentSlot
                    checkable: true
                    text: `Slot ${index + 1}`

                    Component.onCompleted: {
                        // select slot 1 by default
                        checked = (index == 0);
                    }
                }

                onObjectAdded: (index, object) => menuCurrSlot.insertAction(index, object)
                onObjectRemoved: (index, object) => menuCurrSlot.removeAction(object)
            }
        }
    }

    // Dialogs for actions
    Dialogs.FileDialog {
        id: diaOpenRom
        title: qsTr("Open ROM...")
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: [`${qsTr("N64 ROMs")} (*.n64 *.z64 *.v64)`]
        onAccepted: {
            let result = root.core.startROM(selectedFile);
            priv.showDialogForError(result);
        }
    }
    Dialogs.FileDialog {
        id: diaLoadState
        title: qsTr("Load from File")
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: [`${qsTr("Savestates")} (*.st *.savestate)`]
        onAccepted: {
            root.core.loadFile(selectedFile);
        }
    }
    Dialogs.FileDialog {
        id: diaSaveState
        title: qsTr("Save to File")
        fileMode: Dialogs.FileDialog.SaveFile
        nameFilters: [`${qsTr("Savestates")} (*.st *.savestate)`]
        onAccepted: {
            root.core.saveFile(selectedFile);
        }
    }

    Binding {
        // Pause the core if we're interacting with the menu or its items
        root.core.paused: [
            actPause.checked,
            // menu interactions
            root.opened,
            diaLoadState.visible,
            diaSaveState.visible
        ].some(value => value)
        // Tie GS button state to the GSButton item
        root.core.gsButton: actGSButton.checked
    }
    Connections {
        target: root.core

        // reset toggleable actions on startup and shutdown
        function onLaunchedChanged() {
            actPause.checked = false;
            actGSButton.checked = false;
        }
    }
}
