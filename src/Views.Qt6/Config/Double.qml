/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls

import Components as Components

// Qt 6.11 introduces a native DoubleSpinBox. However, we use a custom implementation by Maxim Paperno
// to allow compilation with older versions. This unfortunately has its own issues.
// Ideally we can just bundle Qt 6.11 in an AppImage and call it a day.
Components.DoubleSpinBox {
    property alias target: priv.dummy
    required target

    QtObject {
        id: priv
        property var dummy: null
    }

    value: target
    onValueModified: target = value
}
