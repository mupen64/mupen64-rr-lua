/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QApplication>

#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include "view/MainWindow.hpp"
#include "view/service/QtCoreService.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // start the Qt mainloop
    MainWindow mainWindow;
    mainWindow.show();

    Mupen::core_init(core_cfg{}, std::unique_ptr<Mupen::ICoreService>(new QtCoreService(&mainWindow)));

    return QApplication::exec();
}