/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

pragma Singleton

import QtCore

Settings {
    category: "core"

    enum CoreType {
        CachedInterpreter = 0,
        DynamicRecompiler = 1,
        PureInterpreter = 2
    }

    // Win32 "core" -> core_cfg
    property int coreType: SettingsCore.CoreType.DynamicRecompiler
    property bool stUndoLoad: true
    property int maxLag: 480
    property bool wiiVCEmulation: false
    property bool rcpLagEmulation: false
    property double cpuCF: 1.0
    property double rcpLagFactor: 1.0
    property bool floatExceptionEmulation: false
    property bool useSummercart: false
    property bool stScreenshot: false
    property bool stLZ4: true
    property int romCacheSize: 0

    // Win32 "debug" -> core_cfg
    property bool audioDelayEnabled: true
    property bool compiledJumpEnabled: true
    property bool ceqsNaNAccurate: true
    property bool accurateRDPCompletion: true

    // Win32 "vcr" -> core_cfg
    property bool vcrBackups: true
    property bool vcrWriteExtendedFormat: true

    // Win32 "vcr" (frontend only)
    property bool vcrResetRecordingEnabled: false
}
