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

        Config.Page {
            Config.Row {
                name: "Core type"
                tooltip: "amogus shmamogus"
                // ComboBox {
                //     Layout.preferredWidth: 160
                //     model: [
                //         "Cached Interpreter",
                //         "Dynamic Recompiler",
                //         "Pure Interpreter"
                //     ]
                // }
                Config.Choices {
                    Layout.preferredWidth: 160
                    value: SettingsCore.coreType
                    choices: {
                        "Cached Interpreter": 0,
                        "Dynamic Recompiler": 1,
                        "Pure Interpreter": 2
                    }
                }
            }
        }
    }
}
