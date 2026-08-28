/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "QtIconImageProvider.hpp"
#include <Common/VersionNameHelpers.hpp>
#include <Common.Views/App.hpp>

#include <print>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QtQml/QQmlExtensionPlugin>

Q_IMPORT_QML_PLUGIN(UtilsPlugin)
Q_IMPORT_QML_PLUGIN(CorePlugin)

namespace  {
    using namespace Qt::Literals;
    constexpr QLatin1StringView ORG_DOMAIN = "mupen64.com"_L1;
    constexpr QLatin1StringView ORG_NAME = "Mupen64"_L1;
    constexpr QLatin1StringView DESKTOP_FILE_NAME = "mupen64-rr-lua"_L1;
    constexpr QLatin1StringView DISPLAY_NAME = "Mupen64"_L1;
}

static int qt_main(int argc, char *argv[])
{
    using namespace Qt::Literals;

#ifdef __linux__
    // use xdg-desktop-portal for platform dialogs where possible
    if (qgetenv("QT_QPA_PLATFORMTHEME").isEmpty())
    {
        qputenv("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
    }
#endif
    // TODO: provide and package .desktop file for Linux
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationDomain(ORG_DOMAIN);
    QGuiApplication::setOrganizationName(ORG_NAME);
    QGuiApplication::setApplicationName(DESKTOP_FILE_NAME);
    QGuiApplication::setApplicationVersion(CURRENT_VERSION);
    QGuiApplication::setApplicationDisplayName(DISPLAY_NAME);

    QSettings set;
    std::println("settings path: {}", set.fileName().toStdString());

    QQmlApplicationEngine engine;

    // Close if object creation fails
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [](const QUrl &url) {
            std::println("objectCreationFailed: {}", url.toString().toStdString());
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    // provider for system icons
    engine.addImageProvider(u"icons"_s, new QtIconImageProvider);

    // load and run Views/MainWindow.qml
    engine.loadFromModule("Views", "MainWindow");

    return QGuiApplication::exec();
}

int main(int argc, char *argv[])
{
    return qt_main(argc, argv);
}
