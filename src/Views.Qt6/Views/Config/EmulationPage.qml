/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Components
import Config

ListPage {
    id: root
    required property SettingsCore settingsCore

    delegate: ListPageItem {
        dataSource: root.settingsCore
        itemWidth: root.itemWidth
    }
    model: [
        {
            //% "Core Type"
            key: QT_TRID_NOOP("config.core.coreType"),
            type: ListPageItem.Choices,
            choices: [
                { text: "Cached Interpreter", value: 0 },
                { text: "Dynamic Recompiler", value: 1 },
                { text: "Pure Interpreter", value: 2 },
            ]
        },
        {
            //% "Savestate Undo Load"
            key: QT_TRID_NOOP("config.core.stUndoLoad"),
            type: ListPageItem.Bool,
        },
        {
            //% "Max Lag"
            key: QT_TRID_NOOP("config.core.maxLag"),
            type: ListPageItem.Int,
            from: 0,
            to: 1000,
            stepSize: 10,
        },
        {
            //% "Wii VC Emulation"
            key: QT_TRID_NOOP("config.core.wiiVCEmulation"),
            type: ListPageItem.Bool,
        },
        {
            //% "RCP Lag Emulation"
            key: QT_TRID_NOOP("config.core.rcpLagEmulation"),
            type: ListPageItem.Bool,
        },
        {
            //% "CPU Counter Factor"
            key: QT_TRID_NOOP("config.core.cpuCF"),
            type: ListPageItem.Double,
            from: 1.0,
            to: 4.0,
            stepSize: 0.1,
            decimals: 2,
        },
        {
            //% "RCP Lag Factor"
            key: QT_TRID_NOOP("config.core.rcpLagFactor"),
            type: ListPageItem.Double,
            from: 1.0,
            to: 4.0,
            stepSize: 0.1,
            decimals: 2,
        },
        {
            //% "Float Exception Emulation"
            key: QT_TRID_NOOP("config.core.floatExceptionEmulation"),
            type: ListPageItem.Bool,
        },
        {
            //% "Use Summercart"
            key: QT_TRID_NOOP("config.core.useSummercart"),
            type: ListPageItem.Bool,
        },
        {
            //% "Save Screenshot"
            key: QT_TRID_NOOP("config.core.stScreenshot"),
            type: ListPageItem.Bool,
        },
        {
            //% "Save using LZ4"
            key: QT_TRID_NOOP("config.core.stLZ4"),
            type: ListPageItem.Bool,
        },
        {
            //% "ROM Cache Size"
            key: QT_TRID_NOOP("config.core.romCacheSize"),
            type: ListPageItem.Int,
            from: 0,
            to: 10,
            stepSize: 1,
        }
    ]
}
