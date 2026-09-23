/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQml.Models

Instantiator {
    id: root
    required property ActionManager parent
    readonly property int baseIndex: (parent != null) ? parent.children.findIndex(c => c === root) + 1 : -1

    active: parent != null

    onObjectAdded: (index, obj) => parent.children.splice(baseIndex + index, 0, obj)
    onObjectRemoved: (index, obj) => parent.children.splice(baseIndex + index, 1)
}
