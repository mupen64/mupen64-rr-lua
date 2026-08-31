/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels

import Components

DialogBase {
    id: dialog
    popupType: Popup.Window
    modal: true

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

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                id: pageRoot
                width: Math.max(scroll.width, 300)

                ConfigRow {
                    name: "Core type"
                    tooltip: "amogus shmamogus"
                    ComboBox {
                        Layout.preferredWidth: 160
                        model: [
                            "Cached Interpreter",
                            "Dynamic Recompiler",
                            "Pure Interpreter"
                        ]
                    }
                }
            }
        }
    }
}
