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

ScrollView {
    id: root
    required property SettingsActions settingsActions

    contentWidth: availableWidth
    implicitWidth: 320
    clip: true

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    ListView {
        id: list
        model: root.settingsActions.actions

        width: 310
        spacing: 10

        flickableDirection: Flickable.VerticalFlick

        delegate: Config.Row {
            id: row
            required property EmuAction modelData

            width: list.width

            label.leftPadding: 10
            name: modelData.text

            Config.Hotkey {
                dialog: diaHotkey
                rightPadding: 10
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
    }

    Config.HotkeyDialog {
        id: diaHotkey
    }
}
