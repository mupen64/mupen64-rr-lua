/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: button
    Layout.fillWidth: true
    Layout.fillHeight: true
    implicitWidth: 1.1 * implicitHeight
    checkable: true

    property string label
    property bool rapidFire

    text: rapidFire? `[${label}]` : label

    function doPollUpdate() {
        if (rapidFire) {
            // toggle() doesn't trigger the toggled() signal, we trigger it manually
            toggle();
            toggled();
        }
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onTapped: function(point, mouseBtn) {
            if (mouseBtn == Qt.RightButton) {
                button.rapidFire = true;
                button.toggle();
                button.toggled();
            } else {
                button.rapidFire = false;
            }
        }
    }
}
