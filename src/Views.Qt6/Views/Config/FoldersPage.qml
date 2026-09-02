/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Dialogs as Dialogs
import Config as Config

Config.Page {
    Config.FolderPath {
        title: "ROM Directory"
        target: SettingsPaths.romDir

        Dialogs.FolderDialog {
            title: qsTr("Select ROM Directory")
        }
    }
    Config.FolderPath {
        title: "Save Directory"
        target: SettingsPaths.saveDir

        Dialogs.FolderDialog {
            title: qsTr("Select ROM Directory")
        }
    }
    Config.FolderPath {
        title: "Screenshot Directory"
        target: SettingsPaths.screenshotDir

        Dialogs.FolderDialog {
            title: qsTr("Select ROM Directory")
        }
    }
    Config.FolderPath {
        title: "Backup Directory"
        target: SettingsPaths.backupDir

        Dialogs.FolderDialog {
            title: qsTr("Select ROM Directory")
        }
    }
}
