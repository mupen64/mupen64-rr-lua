/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts

import Components

// Single-row option with a control on the right side.
// Designed for use with Config.Bool, Config.Choices, Config.Int, and Config.Double.
RowLayout {
    id: row
    required property string name
    property string tooltip
    default required property Item control

    Layout.fillWidth: true
    Layout.minimumHeight: 30

    ToolTipLabel {
        Layout.fillWidth: true
        text: row.name
        tooltip: row.tooltip
    }

    // We can't declaratively make it a child, so
    // reattach every time this changes.
    onControlChanged: {
        control.parent = row
    }
}
