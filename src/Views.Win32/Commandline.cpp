/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ThreadPool.h>
#include <Config.h>
#include <DialogService.h>
#include <Messenger.h>
#include <argh.h>
#include <json.hpp>
#include <capture/EncodingManager.h>
#include <Commandline.h>
#include <Loggers.h>
#include <Main.h>
#include <features/Benchmark.h>
#include <features/Compare.h>
#include <features/Dispatcher.h>
#include <lua/LuaConsole.h>

namespace Cli
{
    static std::filesystem::path commandline_rom;
    static std::filesystem::path commandline_lua;
    static std::filesystem::path commandline_st;
    static std::filesystem::path commandline_movie;
    static std::filesystem::path commandline_avi;
    static std::filesystem::path commandline_benchmark;
    static bool commandline_close_on_movie_end;
    static bool commandline_wait_for_debugger;

    static bool dacrate_changed;
    static bool rom_is_movie;
    static bool is_movie_from_start;
    static size_t dacrate_change_count = 0;
    static bool first_emu_launched = true;

    static void start_rom()
    {
        if (commandline_rom.empty())
        {
            return;
        }

        ThreadPool::submit_task([] {
            if (rom_is_movie)
            {
                g_view_logger->trace("[CLI] commandline_start_rom core_vcr_start_playback");
                const auto result = core_vcr_start_playback(commandline_rom);
                show_error_dialog_for_result(result);
            }
            else
            {
                g_view_logger->trace("[CLI] commandline_start_rom vr_start_rom");
                const auto result = core_vr_start_rom(commandline_rom);
                show_error_dialog_for_result(result);
            }
        });
    }

    static void play_movie()
    {
        if (commandline_movie.empty())
            return;

        g_config.core.vcr_readonly = true;
        auto result = core_vcr_start_playback(commandline_movie);
        show_error_dialog_for_result(result);
    }

    static void load_st()
    {
        if (commandline_st.empty())
        {
            return;
        }

        core_st_do_file(commandline_st.c_str(), core_st_job_load, nullptr, false);
    }

    static void start_lua()
    {
        if (commandline_lua.empty())
        {
            return;
        }

        g_main_window_dispatcher->invoke([] {
            // To run multiple lua scripts, a semicolon-separated list is provided
            std::wstringstream stream;
            std::wstring script;
            stream << commandline_lua.wstring();
            while (std::getline(stream, script, L';'))
            {
                lua_create_and_run(script);
            }
        });
    }

    static void start_capture()
    {
        if (commandline_avi.empty())
        {
            return;
        }

        EncodingManager::start_capture(commandline_avi.string().c_str(), static_cast<cfg_encoder_type>(g_config.encoder_type), false);
    }

    static void on_movie_playback_stop()
    {
        if (!commandline_close_on_movie_end)
        {
            return;
        }

        if (!commandline_avi.empty())
        {
            EncodingManager::stop_capture([](auto result) {
                if (!result)
                    return;
                PostMessage(g_main_hwnd, WM_CLOSE, 0, 0);
            });
        }

        if (!commandline_benchmark.empty())
        {
            Benchmark::t_result result{};
            Benchmark::stop(&result);
            Benchmark::save_result_to_file(commandline_benchmark, result);
            PostMessage(g_main_hwnd, WM_CLOSE, 0, 0);
        }
    }

    static void on_task_changed(std::any data)
    {
        auto value = std::any_cast<core_vcr_task>(data);
        static auto previous_value = value;

        if (task_is_playback(previous_value) && !task_is_playback(value))
        {
            on_movie_playback_stop();
        }

        previous_value = value;
    }

    static void on_core_executing_changed(std::any data)
    {
        auto value = std::any_cast<bool>(data);

        if (!value)
            return;

        if (!first_emu_launched)
        {
            return;
        }

        first_emu_launched = false;

        if (!commandline_benchmark.empty())
        {
            Benchmark::start();
        }

        ThreadPool::submit_task([=] {
            g_view_logger->trace("[CLI] on_core_executing_changed -> load_st");
            load_st();

            g_view_logger->trace("[CLI] on_core_executing_changed -> play_movie");
            play_movie();

            g_view_logger->trace("[CLI] on_core_executing_changed -> start_lua");
            start_lua();
        });
    }

    static void on_app_ready(std::any)
    {
        start_rom();
    }

    static void on_dacrate_changed(std::any)
    {
        ++dacrate_change_count;

        g_view_logger->trace("[Cli] on_dacrate_changed {}x", dacrate_change_count);

        if (is_movie_from_start && dacrate_change_count == 2)
        {
            start_capture();
        }

        if (!is_movie_from_start && dacrate_change_count == 1)
        {
            start_capture();
        }
    }

    void init()
    {
        Messenger::subscribe(Messenger::Message::CoreExecutingChanged, on_core_executing_changed);
        Messenger::subscribe(Messenger::Message::AppReady, on_app_ready);
        Messenger::subscribe(Messenger::Message::TaskChanged, on_task_changed);
        Messenger::subscribe(Messenger::Message::DacrateChanged, on_dacrate_changed);

        argh::parser cmdl(__argc, __argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);

        commandline_rom = cmdl({"--rom", "-g"}, "").str();
        commandline_lua = cmdl({"--lua", "-lua"}, "").str();
        commandline_st = cmdl({"--st", "-st"}, "").str();
        commandline_movie = cmdl({"--movie", "-m64"}, "").str();
        commandline_avi = cmdl({"--avi", "-avi"}, "").str();
        commandline_benchmark = cmdl({"--benchmark", "-b"}, "").str();
        commandline_close_on_movie_end = cmdl["--close-on-movie-end"];
        commandline_wait_for_debugger = cmdl["--wait-for-debugger"] || cmdl["--d"];
        bool compare_control = cmdl["--cmp-ctl"] || cmdl["--compare-control"];
        bool compare_actual = cmdl["--cmp-act"] || cmdl["--compare-actual"];
        std::string compare_interval_str = cmdl({"--cmp-int", "--compare-interval"}, "100").str();
        size_t compare_interval = std::stoi(compare_interval_str);

        if (commandline_wait_for_debugger)
        {
            do
            {
                g_view_logger->trace("[CLI] Waiting for debugger to attach...");
                Sleep(100);
            }
            while (!IsDebuggerPresent());
        }

        // handle "Open With...":
        if (cmdl.size() == 2 && cmdl.params().empty())
        {
            commandline_rom = cmdl[1];
        }

        // COMPAT: Old mupen closes emu when movie ends and avi flag is specified.
        if (!commandline_avi.empty())
        {
            commandline_close_on_movie_end = true;
        }

        if (!commandline_benchmark.empty() && commandline_movie.empty() && commandline_rom.empty())
        {
            DialogService::show_dialog(L"Benchmark flag specified without a movie.\nThe benchmark won't be performed.", L"CLI", fsvc_error);
            commandline_benchmark.clear();
        }

        if (!commandline_benchmark.empty() && !commandline_avi.empty())
        {
            DialogService::show_dialog(L"Benchmark flag specified in combination with AVI.\nThe benchmark won't be performed.", L"CLI", fsvc_error);
            commandline_benchmark.clear();
        }

        if (!commandline_benchmark.empty())
        {
            commandline_close_on_movie_end = true;
        }

        // If an st is specified, a movie mustn't be specified
        if (!commandline_st.empty() && !commandline_movie.empty())
        {
            DialogService::show_dialog(L"Both -st and -m64 options specified in CLI parameters.\nThe -st option will be dropped.", L"CLI", fsvc_error);
            commandline_st.clear();
        }

        if (commandline_close_on_movie_end && g_config.core.is_movie_loop_enabled)
        {
            DialogService::show_dialog(L"Movie loop is not allowed when closing on movie end is enabled.\nThe movie loop option will be disabled.", L"CLI", fsvc_warning);
            g_config.core.is_movie_loop_enabled = false;
        }

        // TODO: Show warning when encoding with lazy lua renderer initialization enabled

        // HACK: When playing a movie from start, the rom will start normally and signal us to do our work via EmuLaunchedChanged.
        // The work is started, but then the rom is reset. At that point, the dacrate changes and breaks the capture in some cases.
        // To avoid this, we store the movie's start flag prior to doing anything, and ignore the first EmuLaunchedChanged if it's set.
        const auto movie_path = commandline_rom.extension() == ".m64" ? commandline_rom : commandline_movie;
        if (!movie_path.empty())
        {
            core_vcr_movie_header hdr{};
            core_vcr_parse_header(movie_path, &hdr);
            is_movie_from_start = hdr.startFlags & MOVIE_START_FROM_NOTHING;
        }

        if (compare_control && compare_actual)
        {
            DialogService::show_dialog(L"Can't activate more than one compare mode at once.\nThe comparison system will be disabled.", L"CLI", fsvc_warning);
            compare_control = false;
            compare_actual = false;
        }

        if (compare_control)
        {
            Compare::start(true, compare_interval);
        }

        if (compare_actual)
        {
            Compare::start(false, compare_interval);
        }

        rom_is_movie = commandline_rom.extension() == ".m64";

        g_view_logger->trace("[CLI] commandline_rom: {}", commandline_rom.string());
        g_view_logger->trace("[CLI] commandline_lua: {}", commandline_lua.string());
        g_view_logger->trace("[CLI] commandline_st: {}", commandline_st.string());
        g_view_logger->trace("[CLI] commandline_movie: {}", commandline_movie.string());
        g_view_logger->trace("[CLI] commandline_avi: {}", commandline_avi.string());
        g_view_logger->trace("[CLI] commandline_benchmark: {}", commandline_benchmark.string());
        g_view_logger->trace("[CLI] commandline_close_on_movie_end: {}", commandline_close_on_movie_end);
    }

    bool wants_fast_forward()
    {
        return !commandline_avi.empty() || !commandline_benchmark.empty();
    }
} // namespace Cli
