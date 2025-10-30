/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <chrono>
#include <qapplication.h>
#include "ui/MainWindow.hpp"
#include "model/Core.hpp"

#include <core_api.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <thread>

spdlog::logger& logger() {
  static spdlog::logger g_logger("mupen64rr");
  return g_logger;
}

void sl_log_trace(const std::string& msg) {
  logger().log(spdlog::level::trace, msg);
}

void sl_log_info(const std::string& msg) {
  logger().log(spdlog::level::info, msg);
}

void sl_log_warn(const std::string& msg) {
  logger().log(spdlog::level::warn, msg);
}

void sl_log_error(const std::string& msg) {
  logger().log(spdlog::level::critical, msg);
}


void mupen_cli_test(const QApplication& app) {
  core_params params = {
    .log_trace = sl_log_trace,
    .log_info = sl_log_info,
    .log_warn = sl_log_warn,
    .log_error = sl_log_error,
  };
  core_ctx* p_ctx = nullptr;

  // we expect this to be the only core created
  core_create(&params, &p_ctx);

  // load a rom from the command line
  p_ctx->vr_start_rom(app.arguments()[1].toStdString());

  std::this_thread::sleep_for(std::chrono::seconds(30));

  p_ctx->vr_close_rom(true);
}

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  mupen_cli_test(app);


  return 0;

  // start the Qt mainloop
  // MainWindow mainWindow;
  // mainWindow.show();
  // return QApplication::exec();
}