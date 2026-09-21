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

Config.Page {
    id: root
    required property SettingsCore settingsCore

    Config.Row {
        name: "Core type"
        // tooltip: "Emulation mode to use."
        ComboBox {
            Layout.preferredWidth: 160
            model: [
                { text: "Cached Interpreter", value: 0 },
                { text: "Dynamic Recompiler", value: 1 },
                { text: "Pure Interpreter", value: 2 },
            ]
            textRole: "text"
            valueRole: "value"

            currentValue: root.settingsCore.coreType
            onActivated: root.settingsCore.coreType = currentValue
        }
    }
    Config.Row {
        name: "Savestate Undo Load"
        Switch {
            checked: root.settingsCore.stUndoLoad
            onClicked: root.settingsCore.stUndoLoad = checked
        }
    }
    Config.Row {
        name: "Max Lag"
        SpinBox {
            from: 0
            to: 1000
            stepSize: 10

            value: root.settingsCore.maxLag
            onValueModified: root.settingsCore.maxLag = value
        }
    }
    Config.Row {
        name: "Wii VC emulation"
        Switch {
            checked: root.settingsCore.wiiVCEmulation
            onClicked: root.settingsCore.wiiVCEmulation = checked
        }
    }
    Config.Row {
        name: "RCP Lag Emulation"
        Switch {
            checked: root.settingsCore.rcpLagEmulation
            onClicked: root.settingsCore.rcpLagEmulation = checked
        }
    }
    Config.Row {
        name: "CPU Counter Factor"
        FixedPointSpinBox {
            dFrom: 1.0
            dTo: 4.0
            dStepSize: 0.1
            decimals: 2
            
            dValue: root.settingsCore.cpuCF
            onValueModified: root.settingsCore.cpuCF = dValue
        }
    }
    Config.Row {
        name: "RCP Lag Factor"
        FixedPointSpinBox {
            dFrom: 1.0
            dTo: 4.0
            dStepSize: 0.1
            decimals: 2
            
            dValue: root.settingsCore.rcpLagFactor
            onValueModified: root.settingsCore.rcpLagFactor = dValue
        }
    }
    Config.Row {
        name: "Float Exception Emulation"
        Switch {
            checked: root.settingsCore.floatExceptionEmulation
            onClicked: root.settingsCore.floatExceptionEmulation = checked
        }
    }
    Config.Row {
        name: "Use Summercart"
        Switch {
            checked: root.settingsCore.useSummercart
            onClicked: root.settingsCore.useSummercart = checked
        }
    }
    Config.Row {
        name: "Save Screenshot"
        Switch {
            checked: root.settingsCore.stScreenshot
            onClicked: root.settingsCore.stScreenshot = checked
        }
    }
    Config.Row {
        name: "Save using LZ4"
        Switch {
            checked: root.settingsCore.stLZ4
            onClicked: root.settingsCore.stLZ4 = checked
        }
    }
    Config.Row {
        name: "ROM Cache Size"
        SpinBox {
            from: 0
            to: 10
            stepSize: 1

            value: root.settingsCore.romCacheSize
            onValueModified: root.settingsCore.romCacheSize = value
        }
    }
}
