/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

QtObject {
    id: root
    required property string key
    property string text: qsTrId(key)
    property bool addSeparator: false
}
