/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls

SpinBox {
    property alias target: priv.dummy
    required target

    QtObject {
        id: priv
        property var dummy: null
    }

    value: target
    onValueModified: target = value
}
