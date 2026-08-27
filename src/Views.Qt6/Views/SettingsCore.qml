pragma Singleton

import QtCore

Settings {
    category: "core"

    enum CoreType {
        CachedInterpreter = 0,
        DynamicRecompiler = 1,
        PureInterpreter = 2
    }

    // originally "core" group in Win32
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

    // originally "debug" group in Win32
    property bool audioDelayEnabled: true
    property bool compiledJumpEnabled: true
    property bool ceqsNaNAccurate: true
    property bool accurateRDPCompletion: true

    // originally "vcr" group in Win32
    property bool vcrBackups: true
    property bool vcrWriteExtendedFormat: true
    property bool vcrResetRecordingEnabled: false
}
