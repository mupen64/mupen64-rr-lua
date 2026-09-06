/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DefaultPaths.hpp"

#include <filesystem>
#include <QStandardPaths>
#include <QtUtils.hpp>

QString DefaultPaths::romDir()
{
    // TODO: should this path be different?
    auto homeDirQt = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    auto romsPath = QtUtils::to_std_path(homeDirQt) / "ROMs";
    return QtUtils::to_qt_string(romsPath);
}
QString DefaultPaths::saveDir()
{
    // save to data directory where possible
    auto dataDirQt = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    auto savePath = QtUtils::to_std_path(dataDirQt) / "saves";
    std::filesystem::create_directories(savePath);
    return QtUtils::to_qt_string(savePath);
}

QString DefaultPaths::screenshotDir()
{
    // save to data directory where possible
    auto dataDirQt = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    auto screenshotPath = QtUtils::to_std_path(dataDirQt) / "screenshots";
    std::filesystem::create_directories(screenshotPath);
    return QtUtils::to_qt_string(screenshotPath);
}

QString DefaultPaths::backupDir()
{
    // save to data directory where possible
    auto dataDirQt = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    auto backupPath = QtUtils::to_std_path(dataDirQt) / "backups";
    std::filesystem::create_directories(backupPath);
    return QtUtils::to_qt_string(backupPath);
}
