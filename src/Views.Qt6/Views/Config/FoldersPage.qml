/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Dialogs as Dialogs
import Config as Config

Config.Page {
    id: root
    required property SettingsPaths settingsPaths

    Config.FolderPath {
        title: "ROM Directory"
        target: root.settingsPaths.romDir

        Dialogs.FolderDialog {
            title: qsTr("Select ROM Directory")
        }
    }
    Config.FolderPath {
        title: "Save Directory"
        target: root.settingsPaths.saveDir

        Dialogs.FolderDialog {
            title: qsTr("Select ROM Directory")
        }
    }
    Config.FolderPath {
        title: "Screenshot Directory"
        target: root.settingsPaths.screenshotDir

        Dialogs.FolderDialog {
            title: qsTr("Select ROM Directory")
        }
    }
    Config.FolderPath {
        title: "Backup Directory"
        target: root.settingsPaths.backupDir

        Dialogs.FolderDialog {
            title: qsTr("Select ROM Directory")
        }
    }
}
