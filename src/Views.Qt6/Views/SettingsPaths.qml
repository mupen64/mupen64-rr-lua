pragma Singleton

import QtCore
import QtQuick

import Core

Settings {
    category: "paths"

    property string romDir: DefaultPaths.romDir()
    property string saveDir: DefaultPaths.saveDir()
    property string screenshotDir: DefaultPaths.screenshotDir()
    property string backupDir: DefaultPaths.backupDir()
}
