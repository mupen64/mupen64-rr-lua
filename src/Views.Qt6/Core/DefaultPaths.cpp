#include "DefaultPaths.hpp"

#include <filesystem>
#include <QStandardPaths>

QString DefaultPaths::romDir()
{
    // TODO: should this path be different?
    auto homeDirQt = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    auto romsPath = std::filesystem::path(homeDirQt.toStdU16String()) / "ROMs";
    return QString::fromStdU16String(romsPath.u16string());
}

QString DefaultPaths::saveDir()
{
    // save to data directory where possible
    auto dataDirQt = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    auto savePath = std::filesystem::path(dataDirQt.toStdU16String()) / "saves";
    std::filesystem::create_directories(savePath);
    return QString::fromStdU16String(savePath.u16string());
}

QString DefaultPaths::screenshotDir()
{
    // save to data directory where possible
    auto dataDirQt = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    auto screenshotPath = std::filesystem::path(dataDirQt.toStdU16String()) / "screenshots";
    std::filesystem::create_directories(screenshotPath);
    return QString::fromStdU16String(screenshotPath.u16string());
}

QString DefaultPaths::backupDir()
{
    // save to data directory where possible
    auto dataDirQt = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    auto backupPath = std::filesystem::path(dataDirQt.toStdU16String()) / "backups";
    std::filesystem::create_directories(backupPath);
    return QString::fromStdU16String(backupPath.u16string());
}
