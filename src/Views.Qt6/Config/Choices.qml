/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls

ComboBox {
    id: control
    property alias value: priv.dummy
    required value

    required property var choices

    QtObject {
        id: priv
        property var dummy: null
        property var keyList: Object.keys(control.choices)
        property var valueList: Object.entries(control.choices).map(([key, value]) => value)
        property var valueIndices: {
            let result = new Map();
            let counter = 0;
            for (const [key, value] of Object.entries(control.choices)) {
                result[value] = counter++;
            }
            return result;
        }
    }

    model: priv.keyList
    currentIndex: priv.valueIndices[value]
    onActivated: (index) => value = priv.valueList[index]
}
