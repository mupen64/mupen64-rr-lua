/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls

Dialog {
    id: dialog
    popupType: Popup.Window
    modal: true

    property bool windowResizable

    Connections {
        enabled: dialog.contentItem != null
        target: dialog.contentItem

        function onVisibleChanged() {
            let window = dialog.contentItem.Window.window;
            if (!dialog.contentItem.visible || window == null) return;

            window.minimumWidth = dialog.width;
            window.minimumHeight = dialog.height;

            if (!windowResizable) {
                window.maximumWidth = Qt.binding(() => window.minimumWidth);
                window.maximumHeight = Qt.binding(() => window.minimumHeight);
            }
        }
    }
}
