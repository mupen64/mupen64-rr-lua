/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Config as Config

// Scrollable column.
ScrollView {
    id: root

    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.minimumWidth: 350

    default property list<Item> rows

    ColumnLayout {
        Layout.leftMargin: 10
        Layout.rightMargin: 10
        Layout.topMargin: 20
        id: pageRoot
        width: root.width
        children: root.rows
    }
}
