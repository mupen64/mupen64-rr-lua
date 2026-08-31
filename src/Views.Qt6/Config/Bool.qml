/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls

Switch {
    property alias value: priv.dummy
    required value

    QtObject {
        id: priv
        property var dummy: null
    }

    checked: value
    onClicked: value = checked
}
