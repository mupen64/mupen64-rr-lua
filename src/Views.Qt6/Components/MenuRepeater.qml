/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQml.Models

Instantiator {
    id: root
    required property Menu parent
    property QtObject baseItem: null
    readonly property int baseIndex: {
        if (parent == null)
            return -1;

        // find index of said visual item
        let baseIndex = 0;
        if (baseItem !== null) {
            let checkCallback = (baseItem instanceof Item)?
                (index => parent.itemAt(index) === baseItem) :
                (index => parent.actionAt(index) === baseItem);

            for (let i = 0; i < parent.count; i++) {
                if (checkCallback(i)) {
                    baseIndex = i + 1;
                    break;
                }
            }
        }

        return baseIndex;
    }

    active: parent != null

    onObjectAdded: (index, obj) => {
        parent.insertItem(baseIndex + index, obj);
    }
    onObjectRemoved: (index, obj) => {
        parent.removeItem(obj);
    }
}
