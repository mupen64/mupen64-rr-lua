/***************************************************************************
                                                  commandline.c  -  description
                                                         -------------------
        copyright            : (C) 2003 by ShadowPrince
        email                : shadow@emulation64.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// Based on code from 1964 by Schibo and Rice
// Slightly improved command line params parsing function to work with spaced
// arguments
#include "Commandline.h"
#include <Windows.h>

#include "../../lib/argh.h"
#include "Loggers.h"
#include "Main.h"
#include <core/memory/savestates.h>
#include <core/r4300/r4300.h>
#include <core/r4300/vcr.h>
#include <shared/Messenger.h>
#include <shared/helpers/StlExtensions.h>
#include <thread>
#include <view/capture/EncodingManager.h>
#include <view/lua/LuaConsole.h>

#include "features/Dispatcher.h"
#include "shared/AsyncExecutor.h"
#include "shared/Config.hpp"
#include "shared/services/FrontendService.h"

namespace Cli {
static std::filesystem::path commandline_rom;
static std::filesystem::path commandline_lua;
static std::filesystem::path commandline_st;
static std::filesystem::path commandline_movie;
static std::filesystem::path commandline_avi;
static bool commandline_close_on_movie_end;
static bool dacrate_changed;
static bool rom_is_movie;
static bool is_movie_from_start;
static size_t dacrate_change_count = 0;
static bool first_emu_launched = true;

static void start_rom() {
  if (commandline_rom.empty()) {
    return;
  }

  AsyncExecutor::invoke_async([] {
    if (rom_is_movie) {
      g_view_logger->trace("[CLI] commandline_start_rom VCR::start_playback");
      const auto result = VCR::start_playback(commandline_rom);
      show_error_dialog_for_result(result);
    } else {
      g_view_logger->trace("[CLI] commandline_start_rom vr_start_rom");
      const auto result = vr_start_rom(commandline_rom);
      show_error_dialog_for_result(result);
    }
  });
}

static void play_movie() {
  if (commandline_movie.empty())
    return;

  g_config.vcr_readonly = true;
  auto result = VCR::start_playback(commandline_movie);
  show_error_dialog_for_result(result);
}

static void load_st() {
  if (commandline_st.empty()) {
    return;
  }

  Savestates::do_file(commandline_st.c_str(), Savestates::Job::Load);
}

static void start_lua() {
  if (commandline_lua.empty()) {
    return;
  }

  g_main_window_dispatcher->invoke([] {
    // To run multiple lua scripts, a semicolon-separated list is provided
    std::wstringstream stream;
    std::wstring script;
    stream << commandline_lua.wstring();
    while (std::getline(stream, script, L';')) {
      lua_create_and_run(script);
    }
  });
}

static void start_capture() {
  if (commandline_avi.empty()) {
    return;
  }

  g_main_window_dispatcher->invoke([] {
    EncodingManager::start_capture(
        commandline_avi.string().c_str(),
        static_cast<EncoderType>(g_config.encoder_type), false);
  });
}

static void on_movie_playback_stop() {
  if (commandline_close_on_movie_end) {
    g_main_window_dispatcher->invoke([] {
      EncodingManager::stop_capture();
      PostMessage(g_main_hwnd, WM_CLOSE, 0, 0);
    });
  }
}

static void on_task_changed(std::any data) {
  auto value = std::any_cast<e_task>(data);
  static auto previous_value = value;

  if (task_is_playback(previous_value) && !task_is_playback(value)) {
    on_movie_playback_stop();
  }

  previous_value = value;
}

static void on_core_executing_changed(std::any data) {
  auto value = std::any_cast<bool>(data);

  if (!value)
    return;

  if (!first_emu_launched) {
    return;
  }

  first_emu_launched = false;

  AsyncExecutor::invoke_async([=] {
    g_view_logger->trace("[CLI] on_core_executing_changed -> load_st");
    load_st();

    g_view_logger->trace("[CLI] on_core_executing_changed -> play_movie");
    play_movie();

    g_view_logger->trace("[CLI] on_core_executing_changed -> start_lua");
    start_lua();
  });
}

static void on_app_ready(std::any) { start_rom(); }

static void on_dacrate_changed(std::any) {
  ++dacrate_change_count;

  g_view_logger->trace("[Cli] on_dacrate_changed {}x", dacrate_change_count);

  if (is_movie_from_start && dacrate_change_count == 2) {
    start_capture();
  }

  if (!is_movie_from_start && dacrate_change_count == 1) {
    start_capture();
  }
}

void init() {
  Messenger::subscribe(Messenger::Message::CoreExecutingChanged,
                       on_core_executing_changed);
  Messenger::subscribe(Messenger::Message::AppReady, on_app_ready);
  Messenger::subscribe(Messenger::Message::TaskChanged, on_task_changed);
  Messenger::subscribe(Messenger::Message::DacrateChanged, on_dacrate_changed);

  argh::parser cmdl(__argc, __argv,
                    argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);

  commandline_rom = cmdl({"--rom", "-g"}, "").str();
  commandline_lua = cmdl({"--lua", "-lua"}, "").str();
  commandline_st = cmdl({"--st", "-st"}, "").str();
  commandline_movie = cmdl({"--movie", "-m64"}, "").str();
  commandline_avi = cmdl({"--avi", "-avi"}, "").str();
  commandline_close_on_movie_end = cmdl["--close-on-movie-end"];

  // handle "Open With...":
  if (cmdl.size() == 2 && cmdl.params().empty()) {
    commandline_rom = cmdl[1];
  }

  // COMPAT: Old mupen closes emu when movie ends and avi flag is specified.
  if (!commandline_avi.empty()) {
    commandline_close_on_movie_end = true;
  }

  // If an st is specified, a movie mustn't be specified
  if (!commandline_st.empty() && !commandline_movie.empty()) {
    FrontendService::show_dialog(
        L"Both -st and -m64 options specified in CLI parameters.\nThe -st "
        L"option will be dropped.",
        L"CLI", FrontendService::DialogType::Error);
    commandline_st.clear();
  }

  if (commandline_close_on_movie_end && g_config.is_movie_loop_enabled) {
    FrontendService::show_dialog(
        L"Movie loop is not allowed when closing on movie end is enabled.\nThe "
        L"movie loop option will be disabled.",
        L"CLI", FrontendService::DialogType::Warning);
    g_config.is_movie_loop_enabled = false;
  }

  // HACK: When playing a movie from start, the rom will start normally and
  // signal us to do our work via EmuLaunchedChanged. The work is started, but
  // then the rom is reset. At that point, the dacrate changes and breaks the
  // capture in some cases. To avoid this, we store the movie's start flag prior
  // to doing anything, and ignore the first EmuLaunchedChanged if it's set.
  const auto movie_path = commandline_rom.extension() == ".m64"
                              ? commandline_rom
                              : commandline_movie;
  if (!movie_path.empty()) {
    t_movie_header hdr{};
    VCR::parse_header(movie_path, &hdr);
    is_movie_from_start = hdr.startFlags & MOVIE_START_FROM_NOTHING;
  }

  rom_is_movie = commandline_rom.extension() == ".m64";

  g_view_logger->trace("[CLI] commandline_rom: {}", commandline_rom.string());
  g_view_logger->trace("[CLI] commandline_lua: {}", commandline_lua.string());
  g_view_logger->trace("[CLI] commandline_st: {}", commandline_st.string());
  g_view_logger->trace("[CLI] commandline_movie: {}",
                       commandline_movie.string());
  g_view_logger->trace("[CLI] commandline_avi: {}", commandline_avi.string());
  g_view_logger->trace("[CLI] commandline_close_on_movie_end: {}",
                       commandline_close_on_movie_end);
}

bool wants_fast_forward() { return !commandline_avi.empty(); }
} // namespace Cli
