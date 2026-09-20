/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Components
import Config as Config

ApplicationWindow {
    id: dialog
    modality: Qt.WindowModal

    required property SettingsCore settingsCore
    required property SettingsPaths settingsPaths

    // ensure window fits content
    minimumWidth: mainStack.implicitWidth + leftPadding + rightPadding
    // height: mainStack.implicitHeight

    header: TabBar {
        id: tabs
        TabButton {
            text: qsTr("Emulation")
        }
        TabButton {
            text: qsTr("Folders")
        }
    }

    // windowResizable: true

    StackLayout {
        id: mainStack
        anchors.fill: parent
        currentIndex: tabs.currentIndex

        ConfigEmulationPage {
            settingsCore: dialog.settingsCore
        }
        ConfigFoldersPage {
            settingsPaths: dialog.settingsPaths
        }
    }
}
