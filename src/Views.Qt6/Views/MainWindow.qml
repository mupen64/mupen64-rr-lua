/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Mupen64.Actions
import Mupen64.Core

ApplicationWindow {
    id: window
    visible: true
    //% "mupen64-rr"
    title: qsTrId("appName")

    // WINDOW SIZE
    // =====================================

    // ensure window fits content
    minimumWidth: root.implicitWidth + leftPadding + rightPadding
    minimumHeight: root.implicitHeight + topPadding + bottomPadding

    // lock the window size when the emulator is running
    Binding {
        // minSize check is needed to ensure that everything is actually set
        when: core.launched && window.minimumWidth > 0 && window.minimumHeight > 0
        window.maximumWidth: window.minimumWidth
        window.maximumHeight: window.minimumHeight
    }

    // INITIALIZATION
    // =====================================
    Component.onCompleted: {
        width = 640;
        height = 480;

        // sync settings
        settingsCore.sync();
        settingsPaths.sync();

        // rebuild menu bar
        menuBar.rebuildMenu();
    }

    // MENU BAR
    // =====================================

    header: ActionMenuBar {
        id: menuBar
        actions: settingsActions
    }

    // CONTENT VIEW
    // =====================================

    StackLayout {
        id: root
        anchors.fill: parent

        currentIndex: (core.launched) ? 1 : 0

        RomBrowser {
            context: core
            romManager: romList

            Layout.fillHeight: true
            Layout.fillWidth: true
        }
        Item {
            // All children in the game view will be fixed in size.
            // Use their bounding box as the minimum size.
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height

            EmuDisplay {
                id: coreDisplay
                context: core
                anchors.top: parent.top
                anchors.left: parent.left
            }
            // TODO: Lua canvas management
        }
    }

    // Core context
    // =====================================

    EmuContext {
        id: core

        // ROM manager
        romManager: romList

        // Graphics integration
        onGfxRequestSize: (width, height) => coreDisplay.reserveSize(width, height)
        onUpdateScreen: coreDisplay.readPixels()

        // Dialog service
        onOpenInfoDialog: function (id: int, title: string, content: string, type: int) {
            dialogService.queueInfoDialog(id, title, content, type);
        }
        onOpenAskDialog: function (id: int, title: string, content: string, type: int) {
            dialogService.queueAskDialog(id, title, content, type);
        }
        onOpenMultiDialog: function (id: int, title: string, content: string, choices: list<string>, type: int) {
            dialogService.queueMultiDialog(id, title, content, choices, type);
        }

        // Config options
        options.coreType: settingsCore.coreType
        options.stUndoLoad: settingsCore.stUndoLoad
        options.maxLag: settingsCore.maxLag
        options.wiiVCEmulation: settingsCore.wiiVCEmulation
        options.rcpLagEmulation: settingsCore.rcpLagEmulation
        options.cpuCF: settingsCore.cpuCF
        options.rcpLagFactor: settingsCore.rcpLagFactor
        options.floatExceptionEmulation: settingsCore.floatExceptionEmulation
        options.useSummercart: settingsCore.useSummercart
        options.stScreenshot: settingsCore.stScreenshot
        options.stLZ4: settingsCore.stLZ4
        options.romCacheSize: settingsCore.romCacheSize
        options.audioDelayEnabled: settingsCore.audioDelayEnabled
        options.compiledJumpEnabled: settingsCore.compiledJumpEnabled
        options.ceqsNaNAccurate: settingsCore.ceqsNaNAccurate
        options.accurateRDPCompletion: settingsCore.accurateRDPCompletion
        options.vcrBackups: settingsCore.vcrBackups
        options.vcrWriteExtendedFormat: settingsCore.vcrWriteExtendedFormat

        // Config paths
        paths.saveDir: settingsPaths.saveDir
        paths.screenshotDir: settingsPaths.screenshotDir
        paths.backupDir: settingsPaths.backupDir


        Component.onDestruction: {
            dialogService.discardDialogs()
        }
    }

    RomManager {
        id: romList


        function reloadWithSettings() {
            reloadRomList(
                String(settingsPaths.romDir),
                false
            );
        }

        Component.onCompleted: {
            reloadWithSettings();
        }
    }

    // invalidateVisuals() must be called on each UI frame to
    // request a new frame from the core
    FrameAnimation {
        running: core.launched
        onTriggered: core.invalidateVisuals()
    }

    // Settings objects
    // =====================================

    SettingsCore { id: settingsCore }
    SettingsPaths { id: settingsPaths }
    SettingsActions {
        id: settingsActions

        // core objects
        core: core
        dialogService: dialogService
        winConfig: winConfig
        menuOpen: menuBar.opened
    }

    // Auxiliary dialogs
    // =====================================

    DialogService {
        id: dialogService
        onDialogFinished: function(id, result) {
            core.finishDialog(id, result);
        }
    }

    ConfigWindow {
        id: winConfig
        settingsActions: settingsActions
        settingsCore: settingsCore
        settingsPaths: settingsPaths
    }
}
