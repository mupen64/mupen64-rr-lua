/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as Dialogs

import Actions
import Core

ActionManager {
    id: root

    required property EmuContext core
    required property DialogService dialogService
    required property ConfigWindow winConfig

    property bool menuOpen: false

    settingsCategory: "hotkeys"

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
            root.dialogService.queueInfoDialog(null, title, message.error, CoreMessageTone.Error);
        }
    }

    // ACTIONS
    // ================================

    // FILE
    EmuAction {
        key: "file/loadROM"
        text: qsTr("Load ROM...")
        defaultShortcut: "Ctrl+O"
        onTriggered: diaOpenRom.open()
    }
    EmuAction {
        key: "file/closeROM"
        text: qsTr("Close ROM")
        defaultShortcut: "Ctrl+W"
        enabled: root.core.launched
        onTriggered: {
            let result = root.core.closeROM();
            priv.showDialogForError(result);
        }
    }
    EmuAction {
        key: "file/resetROM"
        text: qsTr("Reset ROM")
        defaultShortcut: "Ctrl+R"
        enabled: root.core.launched
        onTriggered: {
            let result = root.core.resetROM();
            priv.showDialogForError(result);
        }
    }

    // EMULATION (PAUSE/SPEED)
    EmuAction {
        id: actPause
        key: "emu/pause"
        text: qsTr("Pause")
        defaultShortcut: "Pause"
        checkable: true
        enabled: root.core.launched
    }
    EmuAction {
        key: "emu/speedDown"
        text: qsTr("Speed Down")
        // note: Qt represents numpad keys as "Num+[key]"
        // e.g. numpad 0 -> "Num+0"
        defaultShortcut: "Num+-"
        enabled: root.core.launched
        onTriggered: root.core.speedModifier -= 5
    }
    EmuAction {
        key: "emu/speedUp"
        text: qsTr("Speed Up")
        defaultShortcut: "Num++"
        enabled: root.core.launched
        onTriggered: root.core.speedModifier += 5
    }
    EmuAction {
        key: "emu/speedReset"
        text: qsTr("Reset Speed")
        defaultShortcut: "Ctrl+Num++"
        enabled: root.core.launched
        onTriggered: root.core.speedModifier = 100
    }
    EmuHeldAction {
        id: actGSButton
        key: "emu/gsButton"
        defaultShortcut: "G"
        enabled: root.core.launched
        text: qsTr("GS Button")
        checkable: true
    }

    // EMULATION (FRAME ADVANCE)
    EmuAction {
        key: "emu/advance"
        text: qsTr("Frame Advance")
        defaultShortcut: "Num+5"
        enabled: root.core.launched
        onTriggered: {
            actPause.checked = true;
            root.core.frameAdvance(1);
        }
    }
    EmuAction {
        id: actMultiFrameAdvance
        property int frameCount: 0

        key: "emu/multiAdvance"
        text: qsTr("Multi-Frame Advance")
        defaultShortcut: "Ctrl+Num+5"
        enabled: root.core.launched
        onTriggered: {
            if (frameCount == 0) return;
            actPause.checked = true;
            root.core.frameAdvance(frameCount);
        }
    }
    EmuAction {
        key: "emu/multiAdvanceAdd"
        text: qsTr("Multi-Frame Advance +1")
        defaultShortcut: "Ctrl+Q"
        enabled: root.core.launched
        onTriggered: {
            // TODO: should this be capped?
            actMultiFrameAdvance.frameCount += 1;
        }
    }
    EmuAction {
        key: "emu/multiAdvanceSub"
        text: qsTr("Multi-Frame Advance -1")
        defaultShortcut: "Ctrl+E"
        enabled: root.core.launched
        onTriggered: {
            if (actMultiFrameAdvance.frameCount > 0)
                actMultiFrameAdvance.frameCount -= 1;
        }
    }
    EmuAction {
        key: "emu/multiAdvanceReset"
        text: qsTr("Multi-Frame Advance Reset")
        defaultShortcut: "Ctrl+Shift+E"
        enabled: root.core.launched
        onTriggered: {
            // TODO: supply this from config
            actMultiFrameAdvance.frameCount = 0;
        }
    }


    EmuAction {
        key: "emu/saveCurrentSlot"
        text: qsTr("Save Current Slot")
        defaultShortcut: "I"
        enabled: root.core.launched
    }
    EmuAction {
        key: "emu/saveFile"
        text: qsTr("Save as File...")
        enabled: root.core.launched
        onTriggered: diaSaveState.open()
    }
    ActionRepeater {
        parent: root

        model: 10
        delegate: EmuAction {
            required property int index
            key: `emu/saveSlotN/${index}`
            text: `Save Slot ${index + 1}`
            defaultShortcut: "Shift+" + [
                "1", "2", "3", "4", "5",
                "6", "7", "8", "9", "0"
            ][index]
            enabled: root.core.launched
            onTriggered: {
                root.core.saveSlot(index);
            }
        }
    }

    EmuAction {
        key: "emu/loadCurrentSlot"
        text: qsTr("Load Current Slot")
        enabled: root.core.launched
        defaultShortcut: "P"
        onTriggered: {
            let currSlot = groupCurrentSlot.index;
            root.core.saveSlot(currSlot);
        }
    }
    EmuAction {
        key: "emu/loadFile"
        text: qsTr("Load from File...")
        enabled: root.core.launched
        onTriggered: diaLoadState.open()
    }
    ActionRepeater {
        parent: root

        model: 10
        delegate: EmuAction {
            required property int index
            key: `emu/loadSlotN/${index}`
            text: `Load Slot ${index + 1}`
            defaultShortcut: [
                "F1", "F2", "F3", "F4", "F5",
                "F6", "F7", "F8", "F9", "F10"
            ][index]
            enabled: root.core.launched
            onTriggered: {
                root.core.loadSlot(index);
            }
        }
    }

    ActionGroup {
        id: groupCurrentSlot
        readonly property int index: checkedAction.index // qmllint disable missing-property
    }
    ActionRepeater {
        parent: root

        model: 10
        delegate: EmuAction {
            required property int index
            ActionGroup.group: groupCurrentSlot

            checkable: true
            key: `emu/setCurrentSlot/${index}`
            text: `Slot ${index + 1}`
            defaultShortcut: [
                "1", "2", "3", "4", "5",
                "6", "7", "8", "9", "0"
            ][index]
            enabled: root.core.launched

            Component.onCompleted: {
                // select slot 1 by default
                checked = (index == 0);
            }
        }
    }

    EmuAction {
        key: "opts/settings"
        text: qsTr("Settings...")
        defaultShortcut: "Ctrl+S"
        onTriggered: root.winConfig.show()
    }

    // Dialogs
    // ================================

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

    // Bindings
    // ================================

    Binding {
        // Pause the core if we're interacting with the menu or its items
        root.core.paused: [
            actPause.checked,
            // menu interactions
            root.menuOpen,
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
