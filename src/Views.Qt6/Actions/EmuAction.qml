/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Action {
    id: root

    required property string key
    property var defaultShortcut: null
    property bool addSeparator: false

    onKeyChanged: {
        if (!key.startsWith("menu."))
            throw new Error("Action/menu keys must begin with `menu.`");
    }

    text: qsTrId(key)
}
