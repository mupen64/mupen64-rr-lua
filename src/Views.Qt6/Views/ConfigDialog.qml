/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Components
import Config as Config

DialogBase {
    id: dialog

    header: TabBar {
        id: tabs
        TabButton {
            text: qsTr("Folders")
        }
    }

    windowResizable: true

    padding: 10

    StackLayout {
        anchors.fill: parent

        ConfigEmulationPage {}
    }
}
