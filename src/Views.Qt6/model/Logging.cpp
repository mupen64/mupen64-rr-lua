/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Logging.hpp"
#include <boost/dll.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/ansicolor_sink.h>

namespace Mupen {
  static const spdlog::sinks_init_list& sink_init_list() {
    static spdlog::sinks_init_list sinks {
      std::make_shared<spdlog::sinks::basic_file_sink_mt>([]() {
        auto file_path = boost::dll::program_location().parent_path() / "mupen.log";
        return file_path.string();
      }()),
#ifdef _DEBUG
      // ANSI escape codes work on recent Windows
      std::make_shared<spdlog::sinks::ansicolor_stderr_sink_mt>(),
#endif
    };
    return sinks;
  }

  spdlog::logger& core_log() {
    static spdlog::logger logger("core ", sink_init_list());
    return logger;
  }
  spdlog::logger& video_log() {
    static spdlog::logger logger("video", sink_init_list());
    return logger;
  }
  spdlog::logger& audio_log() {
    static spdlog::logger logger("audio", sink_init_list());
    return logger;
  }
  spdlog::logger& input_log() {
    static spdlog::logger logger("input", sink_init_list());
    return logger;
  }
  spdlog::logger& rsp_log() {
    static spdlog::logger logger("rsp  ", sink_init_list());
    return logger;
  }
  spdlog::logger& view_log() {
    static spdlog::logger logger("view ", sink_init_list());
    return logger;
  }
}