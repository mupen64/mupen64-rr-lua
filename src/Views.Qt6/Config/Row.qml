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
Item {
    id: row
    required property string name
    property string tooltip
    default required property Item control

    property alias label: label

    Layout.fillWidth: true

    implicitHeight: Math.max(label.height, control.height, 30)
    implicitWidth: label.width + control.width

    ToolTipLabel {
        id: label
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: row.name
        tooltip: row.tooltip
    }

    // We can't declaratively make it a child, so
    // reattach every time this changes.
    onControlChanged: {
        // parent control
        control.parent = row;
        // bind anchors
        control.anchors.right = row.right;
        control.anchors.verticalCenter = row.verticalCenter;
    }
}
