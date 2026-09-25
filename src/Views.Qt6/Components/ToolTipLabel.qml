/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls

Label {
    property string tooltip

    HoverHandler { id: hoverHandler }

    ToolTip.visible: tooltip && hoverHandler.hovered
    ToolTip.text: tooltip
}
