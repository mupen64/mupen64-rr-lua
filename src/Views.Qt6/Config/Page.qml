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

    contentWidth: availableWidth
    implicitWidth: contentRoot.implicitWidth + ScrollBar.vertical.implicitWidth

    default property list<Item> rows

    ColumnLayout {
        id: contentRoot
        anchors.fill: parent
        ColumnLayout {
            Layout.minimumWidth: 300
            Layout.margins: 10
            children: root.rows
        }
    }

}
