/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Action {
    id: root

    property QtObject _HeldAction_priv: QtObject {
        id: priv

        property var heldShortcutObj: HeldShortcut {
            sequence: root.heldShortcut
        }
    }

    // Similar to shortcut, but for when the action is held.
    property var heldShortcut: null
}
