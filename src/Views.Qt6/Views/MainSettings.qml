pragma Singleton

import QtCore
import QtQuick

Item {
    property alias core: core
    property alias debug: debug
    property alias paths: paths
    property alias vcr: vcr

    Settings {
        id: core
        category: "core"

        // settings below map to core_cfg
        property int coreType
        property bool stUndoLoad
        property bool maxLag
        property bool wiiVCEmulation
        property bool rcpLagEmulation
        property double cpuCF
        property double rcpLagFactor
        property bool floatExceptionEmulation
        property bool useSummercart
        property bool stScreenshot
        property bool stLZ4
        property int romCacheSize
    }

    Settings {
        id: debug
        category: "debug"

        // settings below map to core_cfg.
        property bool audioDelayEnabled
        property bool compiledJumpEnabled
        property bool ceqsNaNAccurate
        property bool accurateRDPCompletion
    }

    Settings {
        id: paths
        category: "paths"

        property url romsDir
        property url saveDir
        property url screenshotsDir
        property url backupsDir
    }

    Settings {
        id: vcr
        category: "vcr"

        property bool backups
        property bool writeExtendedFormat
        property bool resetRecordingEnabled
    }
}
