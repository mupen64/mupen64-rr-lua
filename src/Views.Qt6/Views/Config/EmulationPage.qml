/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import Config as Config

Config.Page {
    id: root
    required property SettingsCore settingsCore

    Config.Row {
        name: "Core type"
        // tooltip: "Emulation mode to use."
        Config.Choices {
            Layout.preferredWidth: 160
            target: root.settingsCore.coreType
            choices: {
                "Cached Interpreter": 0,
                "Dynamic Recompiler": 1,
                "Pure Interpreter": 2
            }
        }
    }
    Config.Row {
        name: "Savestate Undo Load"
        Config.Bool {
            target: root.settingsCore.stUndoLoad
        }
    }
    Config.Row {
        name: "Max Lag"
        Config.Int {
            target: root.settingsCore.maxLag
            from: 0
            to: 1000
            stepSize: 10
        }
    }
    Config.Row {
        name: "Wii VC emulation"
        Config.Bool {
            target: root.settingsCore.wiiVCEmulation
        }
    }
    Config.Row {
        name: "RCP Lag Emulation"
        Config.Bool {
            target: root.settingsCore.rcpLagEmulation
        }
    }
    Config.Row {
        name: "CPU Counter Factor"
        Config.Double {
            target: root.settingsCore.cpuCF
            from: 1.0
            to: 4.0
            stepSize: 0.1
        }
    }
    Config.Row {
        name: "RCP Lag Factor"
        Config.Double {
            target: root.settingsCore.rcpLagFactor
            from: 1.0
            to: 4.0
            stepSize: 0.1
            decimals: 1
        }
    }
    Config.Row {
        name: "Float Exception Emulation"
        Config.Bool {
            target: root.settingsCore.floatExceptionEmulation
        }
    }
    Config.Row {
        name: "Use Summercart"
        Config.Bool {
            target: root.settingsCore.useSummercart
        }
    }
    Config.Row {
        name: "Save Screenshot"
        Config.Bool {
            target: root.settingsCore.stScreenshot
        }
    }
    Config.Row {
        name: "Save using LZ4"
        Config.Bool {
            target: root.settingsCore.stLZ4
        }
    }
    Config.Row {
        name: "ROM Cache Size"
        Config.Int {
            target: root.settingsCore.romCacheSize
            from: 0
            to: 10
            stepSize: 1
        }
    }
}
