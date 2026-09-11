/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick

// Shortcut that can be bound to a held key.
Shortcut {
    id: root
    autoRepeat: false

    property QtObject _priv: QtObject {
        id: priv

        property var allKeys: {
            var allSequences = [root.sequence] + root.sequences;
            return ActionHelpers.sequenceListToKeys(allSequences);
        }

        onAllKeysChanged: {
            HeldShortcutMap.clearShortcuts(root);
            for (const key of allKeys) {
                HeldShortcutMap.addShortcut(key, root);
            }
        }
    }
    // Whether the key(s) are being held or not.
    property bool active
    // True if the current set of sequences is valid.
    // TODO: support multiple keys
    readonly property bool valid: priv.allKeys != null && priv.allKeys.length == 1

    signal released()

    onActivated: {
        if (valid) active = true;
    }
    onReleased: {
        active = false
    }
}
