/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ui/MainWindow.hpp"
#include <chrono>
#include <qapplication.h>

#include <core_api.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <thread>

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  // start the Qt mainloop
  MainWindow mainWindow;
  mainWindow.show();
  return QApplication::exec();
}