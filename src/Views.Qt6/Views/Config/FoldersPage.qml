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
        path: root.settingsPaths.romDir
        onPathModified: root.settingsPaths.romDir = path

        Dialogs.FolderDialog {
            //% "Set ROM Directory..."
            title: qsTrId("dialogs.pickRomDir.title")
        }
    }
    Config.FolderPath {
        title: "Save Directory"
        path: root.settingsPaths.saveDir
        onPathModified: root.settingsPaths.screenshotDir = path

        Dialogs.FolderDialog {
            //% "Set Save Directory..."
            title: qsTrId("dialogs.pickSaveDir.title")
        }
    }
    Config.FolderPath {
        title: "Screenshot Directory"
        path: root.settingsPaths.screenshotDir
        onPathModified: root.settingsPaths.screenshotDir = path

        Dialogs.FolderDialog {
            //% "Set Screenshot Directory..."
            title: qsTrId("dialogs.pickScreenshotDir.title")
        }
    }
    Config.FolderPath {
        title: "Backup Directory"
        path: root.settingsPaths.backupDir
        onPathModified: root.settingsPaths.screenshotDir = path

        Dialogs.FolderDialog {
            //% "Set Backup Directory..."
            title: qsTrId("dialogs.pickBackupDir.title")
        }
    }
}
