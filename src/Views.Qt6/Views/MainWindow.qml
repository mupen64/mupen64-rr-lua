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

    DialogService {
        id: dialogService
    }
}
