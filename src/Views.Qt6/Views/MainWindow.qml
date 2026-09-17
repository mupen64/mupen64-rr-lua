/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Actions
import Core
import Views

ApplicationWindow {
    id: mainWindow
    visible: true
    title: qsTr("Mupen64RR")

    // WINDOW SIZE
    // =====================================

    // ensure window fits content
    minimumWidth: mainStack.implicitWidth + leftPadding + rightPadding
    minimumHeight: mainStack.implicitHeight + topPadding + bottomPadding

    // lock the window size when the emulator is running
    Binding {
        // minSize check is needed to ensure that everything is actually set
        when: core.launched && mainWindow.minimumWidth > 0 && mainWindow.minimumHeight > 0
        mainWindow.maximumWidth: mainWindow.minimumWidth
        mainWindow.maximumHeight: mainWindow.minimumHeight
    }

    // INITIALIZATION
    // =====================================
    Component.onCompleted: {
        settingsCore.sync();
        settingsPaths.sync();
    }

    // MENU BAR
    // =====================================

    header: MainMenuBar {
        core: core
        dialogService: dialogService
        diaConfig: diaConfig
    }

    // CONTENT VIEW
    // =====================================

    StackLayout {
        id: mainStack
        anchors.fill: parent

        currentIndex: (core.launched) ? 1 : 0

        Item {
            // TODO: replace with ROM browser
            Layout.fillHeight: true
            Layout.fillWidth: true
            // Button {
            //     anchors.centerIn: parent
            //     text: "MessageBox test"
            //     onClicked: {
            //         dialogService.queueInfoDialog(null, "Hello there.", "General Kenobi! You are a bold one.", CoreMessageTone.Error)
            //     }
            // }
            TextField {
                anchors.centerIn: parent
            }
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

        // Graphics integration
        onGfxRequestSize: (width, height) => coreDisplay.reserveSize(width, height)
        onUpdateScreen: coreDisplay.readPixels()

        // Dialog service
        onOpenInfoDialog: dialogService.queueInfoDialog
        onOpenAskDialog: dialogService.queueAskDialog
        onOpenMultiDialog: dialogService.queueMultiDialog

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
        paths.romDir: settingsPaths.romDir
        paths.saveDir: settingsPaths.saveDir
        paths.screenshotDir: settingsPaths.screenshotDir
        paths.backupDir: settingsPaths.backupDir
    }

    // invalidateVisuals() must be called on each UI frame to
    // request a new frame from the core
    FrameAnimation {
        running: core.launched
        onTriggered: core.invalidateVisuals()
    }

    // Auxiliary dialogs
    // =====================================

    DialogService { id: dialogService }

    ConfigDialog {
        id: diaConfig
        settingsCore: settingsCore
        settingsPaths: settingsPaths
    }

    // Settings objects
    // =====================================

    SettingsCore { id: settingsCore }
    SettingsPaths { id: settingsPaths }
}
