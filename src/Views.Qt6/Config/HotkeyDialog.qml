/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls

import Actions
import Components

DialogBase {
    id: dialog


    property var currentCombo: null
    property bool allowModifiers: true

    standardButtons: Dialog.Cancel
    onOpened: {
        lblDisplay.lineTwo = "...";
    }

    Item {
        width: 300
        implicitWidth: 300
        Label {
            anchors.centerIn: parent
            id: lblDisplay
            focus: true
            textFormat: Qt.MarkdownText
            text: `Recording: ${lineTwo}`
            horizontalAlignment: Text.AlignHCenter

            property string lineTwo: "..."

            function updateDisplay(key: int, modifiers: int) {
                let combined = modifiers;
                if (![Qt.Key_Control, Qt.Key_Alt, Qt.Key_Shift, Qt.Key_Meta].includes(key)) {
                    combined |= key;
                }
                let combo = ActionHelpers.fromIntKeys([combined]);

                lineTwo = combo;
            }

            Keys.onShortcutOverride: function(event: KeyEvent) {
                event.accepted = true;
            }

            Keys.onPressed: function(event: KeyEvent) {
                event.accepted = true;
                if (event.key == Qt.Key_Escape) {
                    dialog.currentCombo = null;
                    dialog.accept();
                }
                if (event.isAutoRepeat) {
                    return;
                }
                if (!dialog.allowModifiers && event.modifiers !== Qt.NoModifier) {
                    return;
                }

                lblDisplay.updateDisplay(event.key, event.modifiers);

                if ([Qt.Key_Control, Qt.Key_Alt, Qt.Key_Shift, Qt.Key_Meta].includes(event.key)) {
                    return;
                }

                dialog.currentCombo = ActionHelpers.fromIntKeys([event.key | event.modifiers]);
                dialog.accept();
            }
        }
    }

}
