/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Qt.labs.qmlmodels

Dialog {
    id: dialog
    popupType: Popup.Window
    modal: true

    header: TabBar {
        id: tabs
        TabButton {
            text: qsTr("Folders")
        }
    }

    StackLayout {
        id: root
        currentIndex: tabs.currentIndex

        onVisibleChanged: {
            if (visible && Window.window !== null) {
                let window = Window.window;
                window.minimumWidth = Qt.binding(() => {
                    return Math.max(root.implicitWidth, 300);
                });
                window.minimumHeight = Qt.binding(() => {
                    let implicitTotalHeight = root.implicitHeight + dialog.header.implicitHeight + dialog.footer.implicitHeight;
                    return Math.max(implicitTotalHeight, 500);
                });
            }
        }


    }
}
