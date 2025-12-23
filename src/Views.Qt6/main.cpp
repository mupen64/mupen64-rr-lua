/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <iostream>

#include <QApplication>

#include "view/service/QtCoreService.hpp"
#include "view/MainWindow.hpp"

#include <boost/dll/shared_library.hpp>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // start the Qt mainloop
    MainWindow mainWindow;
    mainWindow.show();

    Mupen::core_init(core_cfg{}, new QtCoreService(&mainWindow));

    QObject::connect(&app, &QGuiApplication::lastWindowClosed, []() {
        Mupen::core_drop();
    });

    return QApplication::exec();
}