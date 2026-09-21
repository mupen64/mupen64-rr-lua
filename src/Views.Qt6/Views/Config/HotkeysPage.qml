/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Actions
import Config as Config

ScrollView {
    id: root
    required property SettingsActions settingsActions

    contentWidth: availableWidth
    implicitWidth: 320
    clip: true

    ListView {
        model: root.settingsActions.actions
        spacing: 10
        delegate: Config.Row {
            width: 300

            id: row
            required property EmuAction modelData
            name: modelData.text

            Config.Hotkey {
                combo: row.modelData.shortcut
                onComboModified: row.modelData.shortcut = combo
            }
        }
    }
}
