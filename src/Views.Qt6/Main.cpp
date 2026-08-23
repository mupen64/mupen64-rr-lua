/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "QtIconImageProvider.hpp"
#include <Common.Views/App.hpp>

#include <print>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/QQmlExtensionPlugin>

Q_IMPORT_QML_PLUGIN(CorePlugin)

static int cli_main(int argc, char* argv[]) {
    using namespace std::literals;
    if (argc != 2)
    {
        std::println("usage: {} [path to ROM]", argv[0]);
        return 1;
    }

    // auto context = 

    // auto res1 = Core::context()->vr_start_rom(argv[1]);
    // std::println("result: {}", (int)res1);
    // std::this_thread::sleep_for(10s);
    // Core::context()->vr_close_rom(true);
    return 0;
}

static int qt_main(int argc, char *argv[])
{
    using namespace Qt::Literals;

#ifdef __linux__
    // use xdg-desktop-portal for platform dialogs where possible
    if (qgetenv("QT_QPA_PLATFORMTHEME").isEmpty()) {
        qputenv("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
    }
#endif

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // Close if object creation fails
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [](const QUrl& url) {
        std::println("objectCreationFailed: {}", url.toString().toStdString());
        QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    // provider for system icons
    engine.addImageProvider(u"icons"_s, new QtIconImageProvider);

    // load and run Views/MainWindow.qml
    engine.loadFromModule("Views", "MainWindow");
    
    return QGuiApplication::exec();
}

int main(int argc, char* argv[]) {
    return qt_main(argc, argv);
}
