/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick

import Actions
import Config as Config

Config.ListPage {
    id: root
    required property SettingsActions settingsActions
    model: root.settingsActions.actions

    delegate: Config.Row {
        id: row
        required property EmuAction modelData

        width: root.itemWidth
        name: modelData.text

        Config.Hotkey {
            dialog: diaHotkey
            allowModifiers: !(row.modelData instanceof EmuHeldAction)
            combo: {
                if (row.modelData instanceof EmuHeldAction)
                    return (row.modelData as EmuHeldAction).heldShortcut;
                else
                    return row.modelData.shortcut;
            }
            onComboModified: {
                if (row.modelData instanceof EmuHeldAction) {
                    (row.modelData as EmuHeldAction).heldShortcut = combo;
                }
                else
                    row.modelData.shortcut = combo;
            }
        }
    }

    Config.HotkeyDialog {
        id: diaHotkey
    }
}
