/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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
        SettingsCore.sync()
        SettingsPaths.sync()
    }

    // MENU BAR
    // =====================================

    header: MainMenuBar {
        core: core
        dialogService: dialogService
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
            Button {
                anchors.centerIn: parent
                text: "MessageBox test"
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
    }

    // invalidateVisuals() must be called on each UI frame to
    // request a new frame from the core
    FrameAnimation {
        running: core.launched
        onTriggered: core.invalidateVisuals()
    }

    // Auxiliary compoments
    // =====================================

    DialogService {
        id: dialogService
    }
}
