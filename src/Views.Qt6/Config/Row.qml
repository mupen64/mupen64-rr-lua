/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: row
    required property string name
    required property string tooltip
    default required property Item control

    Layout.fillWidth: true

    ToolTipLabel {
        Layout.fillWidth: true
        text: "Core type"
        tooltip: "amogus shmamogus"
    }

    // We can't declaratively make it a child, so
    // reattach every time this changes.
    onControlChanged: {
        control.parent = row
    }
}
