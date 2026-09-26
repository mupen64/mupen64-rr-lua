/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs


import Actions
import Components

Button {
    id: root
    property var combo: null
    required property HotkeyDialog dialog
    property bool allowModifiers: true

    signal comboModified()

    //% "(none)"
    text: (combo == null) ? qsTrId("misc.noHotkey") : combo

    onClicked: {
        accepter.enabled = true;
        dialog.allowModifiers = root.allowModifiers;
        dialog.open();
    }

    Connections {
        id: accepter
        target: root.dialog
        enabled: false
        function onAccepted() {
            root.combo = root.dialog.currentCombo;
            root.comboModified();
        }
        function onVisibleChanged() {
            if (!root.dialog.visible)
                enabled = false;
        }

    }
}
