/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick

EmuAction {
    id: root

    // Similar to shortcut, but for when the action is held.
    property var heldShortcut: null

    readonly property HeldShortcut shortcutImpl: HeldShortcut {
        id: shortcutImpl
        sequence: root.heldShortcut
        enabled: root.enabled

        onActiveChanged: root.checked = active
    }

    checkable: true
}
