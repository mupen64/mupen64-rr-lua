/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"

#include <Common.Views/App.hpp>

#include <future>
#include <iostream>
#include <print>

#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "Core.hpp"

static int cli_main(int argc, char* argv[]) {
    using namespace std::literals;
    if (argc != 2)
    {
        std::println("usage: {} [path to ROM]", argv[0]);
        return 1;
    }

    auto res1 = Core::context()->vr_start_rom(argv[1]);
    std::println("result: {}", (int)res1);
    std::this_thread::sleep_for(10s);
    Core::context()->vr_close_rom(true);
    return 0;
}

static int qt_main(int argc, char *argv[])
{
#ifdef __linux__
    // use xdg-desktop-portal for platform dialogs where possible
    if (qgetenv("QT_QPA_PLATFORMTHEME").isEmpty()) {
        qputenv("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
    }
#endif

    QGuiApplication app(argc, argv);
    
    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() {
        QCoreApplication::exit(-1);
    });
    engine.loadFromModule("Mupen64RR.UI", "MainWindow");
    
    return QGuiApplication::exec();
}

int main(int argc, char* argv[]) {
    return cli_main(argc, argv);
}
