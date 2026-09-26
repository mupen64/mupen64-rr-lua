/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import Config

ListPage {
    id: root
    required property SettingsPaths settingsPaths

    delegate: ListPageItem {
        dataSource: root.settingsPaths
        itemWidth: root.itemWidth
    }
    model: [
        {
            //% "ROM Directory"
            key: QT_TRID_NOOP("config.paths.romDir"),
            type: ListPageItem.FolderPath,
            //% "Set ROM Directory..."
            dialogTitle: qsTrId("dialogs.pickRomDir.title")
        },
        {
            //% "Save Directory"
            key: QT_TRID_NOOP("config.paths.saveDir"),
            type: ListPageItem.FolderPath,
            //% "Set Save Directory..."
            dialogTitle: qsTrId("dialogs.pickSaveDir.title")
        },
        {
            //% "Screenshot Directory"
            key: QT_TRID_NOOP("config.paths.screenshotDir"),
            type: ListPageItem.FolderPath,
            //% "Set Screenshot Directory..."
            dialogTitle: qsTrId("dialogs.pickScreenshotDir.title")
        },
        {
            //% "ROM Directory"
            key: QT_TRID_NOOP("config.paths.backupDir"),
            type: ListPageItem.FolderPath,
            //% "Set ROM Directory..."
            dialogTitle: qsTrId("dialogs.pickBackupDir.title")
        }
    ]
}
