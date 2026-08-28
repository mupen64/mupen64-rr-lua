/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

DialogBase {
    id: dialog

    header: TabBar {
        id: tabs
        TabButton {
            text: qsTr("Folders")
        }
    }

    minimumWidth: 300
    minimumHeight: 400

    StackLayout {
        id: root
        currentIndex: tabs.currentIndex

        ColumnLayout {
            // TODO: OPTIONS
        }
    }
}
