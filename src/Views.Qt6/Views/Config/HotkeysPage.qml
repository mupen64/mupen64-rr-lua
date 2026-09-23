/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
// import QtQuick.Layouts

import Actions
import Config as Config

ListView {
    id: root
    required property SettingsActions settingsActions
    model: root.settingsActions.actions

    spacing: 10
    leftMargin: 10
    rightMargin: 10 + scrollBar.width

    flickableDirection: Flickable.VerticalFlick
    ScrollBar.vertical: ScrollBar {
        id: scrollBar
        active: true
    }

    clip: true

    delegate: Config.Row {
        id: row
        required property EmuAction modelData

        width: root.width - root.leftMargin - root.rightMargin

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
                if (row.modelData instanceof EmuHeldAction)
                    (row.modelData as EmuHeldAction).heldShortcut = combo;
                else
                    row.modelData.shortcut = combo;
            }
        }
    }

    Config.HotkeyDialog {
        id: diaHotkey
    }
}
