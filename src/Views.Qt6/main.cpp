/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <dlfcn.h>
#include <iostream>

#include <QApplication>

#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include "core_plugin.h"
#include "mupapi.h"
#include "view/service/QtCoreService.hpp"
#include "view/MainWindow.hpp"

#include <boost/dll/shared_library.hpp>


int qt_main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // start the Qt mainloop
    MainWindow mainWindow;
    mainWindow.show();

    Mupen::core_init(core_cfg{}, std::unique_ptr<Mupen::ICoreService>(new QtCoreService(&mainWindow)));

    return QApplication::exec();
}

int test_main(int argc, char *argv[]) {
    auto test = dlopen("/home/jgcodes/Documents/Code/C++/mupen64-rr-lua/build/out/plugin/no-video.so", RTLD_LAZY | RTLD_LOCAL);

    std::cout << "getting mup_get_info()\n";
    auto fp = (fp_mup_get_info) dlsym(test, "mup_get_info");

    std::cout << "calling mup_get_info()\n";
    auto info = core_plugin_info {};

    fp(&info);
    return 0;
}

int main(int argc, char *argv[])
{
    return qt_main(argc, argv);
}