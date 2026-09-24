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

    EmuMenu {
        //% "File"
        key: QT_TRID_NOOP("menu.file")
    }
    EmuAction {
        //% "Load ROM..."
        key: QT_TRID_NOOP("menu.file.loadROM")
        defaultShortcut: "Ctrl+O"
        onTriggered: diaOpenRom.open()
    }
    EmuAction {
        //% "Close ROM"
        key: QT_TRID_NOOP("menu.file.closeROM")
        defaultShortcut: "Ctrl+W"
        enabled: root.core.launched
        onTriggered: {
            let result = root.core.closeROM();
            priv.showDialogForError(result);
        }
    }
    EmuAction {
        //% "Reset ROM"
        key: QT_TRID_NOOP("menu.file.resetROM")
        defaultShortcut: "Ctrl+R"
        enabled: root.core.launched
        onTriggered: {
            let result = root.core.resetROM();
            priv.showDialogForError(result);
        }
    }

    // EMULATION (PAUSE/SPEED)
    EmuMenu {
        //% "Emulation"
        key: QT_TRID_NOOP("menu.emu")
    }
    EmuAction {
        id: actPause
        //% "Pause"
        key: QT_TRID_NOOP("menu.emu.pause")
        defaultShortcut: "Pause"
        checkable: true
        enabled: root.core.launched
    }
    EmuAction {
        //% "Speed Down"
        key: QT_TRID_NOOP("menu.emu.speedDown")
        // note: Qt represents numpad keys as "Num+[key]"
        // e.g. numpad 0 -> "Num+0"
        defaultShortcut: "Num+-"
        enabled: root.core.launched
        onTriggered: root.core.speedModifier -= 5
    }
    EmuAction {
        //% "Speed Up"
        key: QT_TRID_NOOP("menu.emu.speedUp")
        defaultShortcut: "Num++"
        enabled: root.core.launched
        onTriggered: root.core.speedModifier += 5
    }
    EmuAction {
        //% "Reset Speed"
        key: QT_TRID_NOOP("menu.emu.speedReset")
        defaultShortcut: "Ctrl+Num++"
        enabled: root.core.launched
        onTriggered: root.core.speedModifier = 100
    }
    EmuHeldAction {
        id: actFastForward
        //% "Fast Forward"
        key: QT_TRID_NOOP("menu.emu.fastForward")
        defaultShortcut: "`"
        enabled: root.core.launched
    }
    EmuHeldAction {
        id: actGSButton
        //% "GS Button"
        key: QT_TRID_NOOP("menu.emu.gsButton")
        addSeparator: true
        defaultShortcut: "G"
        enabled: root.core.launched
        checkable: true
    }

    // EMULATION (FRAME ADVANCE)
    EmuAction {
        //% "Frame Advance"
        key: QT_TRID_NOOP("menu.emu.advance")
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

        //% "Multi-Frame Advance"
        key: QT_TRID_NOOP("menu.emu.multiAdvance")
        defaultShortcut: "Ctrl+Num+5"
        enabled: root.core.launched
        onTriggered: {
            if (frameCount == 0) return;
            actPause.checked = true;
            root.core.frameAdvance(frameCount);
        }
    }
    EmuAction {
        //% "Multi-Frame Advance +1"
        key: QT_TRID_NOOP("menu.emu.multiAdvanceAdd")
        defaultShortcut: "Ctrl+Q"
        enabled: root.core.launched
        onTriggered: {
            // TODO: should this be capped?
            actMultiFrameAdvance.frameCount += 1;
        }
    }
    EmuAction {
        //% "Multi-Frame Advance -1"
        key: QT_TRID_NOOP("menu.emu.multiAdvanceSub")
        defaultShortcut: "Ctrl+E"
        enabled: root.core.launched
        onTriggered: {
            if (actMultiFrameAdvance.frameCount > 0)
                actMultiFrameAdvance.frameCount -= 1;
        }
    }
    EmuAction {
        //% "Multi-Frame Advance Reset"
        key: QT_TRID_NOOP("menu.emu.multiAdvanceReset")
        addSeparator: true
        defaultShortcut: "Ctrl+Shift+E"
        enabled: root.core.launched
        onTriggered: {
            // TODO: supply this from config
            actMultiFrameAdvance.frameCount = 0;
        }
    }

    EmuMenu {
        //% "Save State"
        key: QT_TRID_NOOP("menu.emu.saveState")
    }
    EmuAction {
        //% "Save Current Slot"
        key: QT_TRID_NOOP("menu.emu.saveState.currentSlot")
        defaultShortcut: "I"
        enabled: root.core.launched
    }
    EmuAction {
        //% "Save as File..."
        key: QT_TRID_NOOP("menu.emu.saveState.file")
        addSeparator: true
        enabled: root.core.launched
        onTriggered: diaSaveState.open()
    }
    ActionRepeater {
        parent: root

        model: 10
        delegate: EmuAction {
            required property int index
            key: `menu.emu.saveState.slot${index}`
            //% "Slot %1"
            text: qsTrId("menu.emu.saveState.slot").arg(index + 1)
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

    EmuMenu {
        //% "Load State"
        key: QT_TRID_NOOP("menu.emu.loadState")
    }
    EmuAction {
        //% "Load Current Slot"
        key: QT_TRID_NOOP("menu.emu.loadState.currentSlot")
        enabled: root.core.launched
        defaultShortcut: "P"
        onTriggered: {
            let currSlot = groupCurrentSlot.index;
            root.core.saveSlot(currSlot);
        }
    }
    EmuAction {
        //% "Load from File..."
        key: QT_TRID_NOOP("menu.emu.loadState.file")
        addSeparator: true
        enabled: root.core.launched
        onTriggered: diaLoadState.open()
    }
    ActionRepeater {
        parent: root

        model: 10
        delegate: EmuAction {
            required property int index
            key: `menu.emu.loadState.slot${index}`
            //% "Slot %1"
            text: qsTrId("menu.emu.loadState.slot").arg(index + 1)
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
    EmuMenu {
        //% "Current State Slot"
        key: QT_TRID_NOOP("menu.emu.currentSlot")
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
            key: `menu.emu.currentSlot.slot${index}`
            //% "Slot %1"
            text: qsTrId("menu.emu.currentState.slot").arg(index + 1)
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

    EmuMenu {
        //% "Options"
        key: QT_TRID_NOOP("menu.opts")
    }
    EmuAction {
        //% "Settings..."
        key: QT_TRID_NOOP("menu.opts.settings")
        defaultShortcut: "Ctrl+S"
        onTriggered: root.winConfig.show()
    }

    // Dialogs
    // ================================

    Dialogs.FileDialog {
        id: diaOpenRom
        //% "Load ROM..."
        title: qsTrId("dialogs.loadROM.title")
        fileMode: Dialogs.FileDialog.OpenFile
        //% "N64 ROMs"
        nameFilters: [`${qsTrId("formats.rom")} (*.n64 *.z64 *.v64)`]
        onAccepted: {
            let result = root.core.startROM(selectedFile);
            priv.showDialogForError(result);
        }
    }
    Dialogs.FileDialog {
        id: diaLoadState
        //% "Load State..."
        title: qsTrId("dialogs.loadState.title")
        fileMode: Dialogs.FileDialog.OpenFile
        //% "Savestates"
        nameFilters: [`${qsTrId("formats.state")} (*.st *.savestate)`]
        onAccepted: {
            root.core.loadFile(selectedFile);
        }
    }
    Dialogs.FileDialog {
        id: diaSaveState
        //% "Save State..."
        title: qsTrId("dialogs.saveState.title")
        fileMode: Dialogs.FileDialog.SaveFile
        nameFilters: [`${qsTrId("formats.state")} (*.st *.savestate)`]
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

        // Tie fast-forward state to the checked item
        root.core.speedMode: (actFastForward.checked)? CoreSpeedMode.FastForward : CoreSpeedMode.Normal
    }
    Connections {
        target: root.core

        // reset toggleable actions on startup and shutdown
        function onLaunchedChanged() {
            actPause.checked = false;
            actGSButton.checked = false;
            actFastForward.checked = false;
        }
    }
}
