/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
