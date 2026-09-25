/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

// Common setup for a vertical ListView.
ListView {
    id: root

    property double itemWidth: width - leftMargin - rightMargin

    spacing: 10
    leftMargin: 10
    rightMargin: 10 + ((scrollBar.visible)? scrollBar.width : 0)

    flickableDirection: Flickable.VerticalFlick
    ScrollBar.vertical: ScrollBar {
        id: scrollBar
        policy: ScrollBar.AsNeeded
    }
    clip: true
}
