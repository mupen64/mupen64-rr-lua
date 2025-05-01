/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Config.h>
#include <DialogService.h>
#include <Main.h>
#include <Messenger.h>
#include <ThreadPool.h>
#include <argh.h>
#include <json.hpp>
#include <capture/EncodingManager.h>
#include <components/Benchmark.h>
#include <components/CLI.h>
#include <components/Compare.h>
#include <components/Dispatcher.h>
#include <lua/LuaConsole.h>

struct t_cli_params {
    std::filesystem::path rom{};
    std::filesystem::path lua{};
    std::filesystem::path st{};
    std::filesystem::path m64{};
    std::filesystem::path avi{};
    std::filesystem::path benchmark{};
    bool close_on_movie_end{};
    bool wait_for_debugger{};
};

struct t_cli_state {
    bool rom_is_movie{};
    bool is_movie_from_start{};
    size_t dacrate_change_count{};
    bool first_emu_launched = true;
};

static t_cli_params cli_params{};
static t_cli_state cli_state{};

static void log_cli_params(const t_cli_params& params)
{
    g_view_logger->trace("log_cli_params:");
    g_view_logger->trace("  rom: {}", params.rom.string());
    g_view_logger->trace("  lua: {}", params.lua.string());
    g_view_logger->trace("  st: {}", params.st.string());
    g_view_logger->trace("  m64: {}", params.m64.string());
    g_view_logger->trace("  avi: {}", params.avi.string());
    g_view_logger->trace("  benchmark: {}", params.benchmark.string());
    g_view_logger->trace("  close_on_movie_end: {}", params.close_on_movie_end);
    g_view_logger->trace("  wait_for_debugger: {}", params.wait_for_debugger);
}

static void start_rom()
{
    if (cli_params.rom.empty())
    {
        return;
    }

    ThreadPool::submit_task([] {
        if (!cli_state.rom_is_movie)
        {
            const auto result = core_vr_start_rom(cli_params.rom);
            show_error_dialog_for_result(result);
            return;
        }

        const auto result = core_vcr_start_playback(cli_params.rom);
        show_error_dialog_for_result(result);
    });
}

static void play_movie()
{
    if (cli_params.m64.empty())
        return;

    g_config.core.vcr_readonly = true;
    auto result = core_vcr_start_playback(cli_params.m64);
    show_error_dialog_for_result(result);
}

static void load_st()
{
    if (cli_params.st.empty())
    {
        return;
    }

    core_st_do_file(cli_params.st.c_str(), core_st_job_load, nullptr, false);
}

static void start_lua()
{
    if (cli_params.lua.empty())
    {
        return;
    }

    g_main_window_dispatcher->invoke([] {
        // To run multiple lua scripts, a semicolon-separated list is provided
        std::wstringstream stream;
        std::wstring script;
        stream << cli_params.lua.wstring();
        while (std::getline(stream, script, L';'))
        {
            lua_create_and_run(script);
        }
    });
}

static void start_capture()
{
    if (cli_params.avi.empty())
    {
        return;
    }

    EncodingManager::start_capture(cli_params.avi.string().c_str(), static_cast<t_config::EncoderType>(g_config.encoder_type), false);
}

static void on_movie_playback_stop()
{
    if (!cli_params.close_on_movie_end)
    {
        return;
    }

    if (!cli_params.avi.empty())
    {
        EncodingManager::stop_capture([](auto result) {
            if (!result)
                return;
            PostMessage(g_main_hwnd, WM_CLOSE, 0, 0);
        });
    }

    if (!cli_params.benchmark.empty())
    {
        Benchmark::t_result result{};
        Benchmark::stop(&result);
        Benchmark::save_result_to_file(cli_params.benchmark, result);
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

    if (!cli_state.first_emu_launched)
    {
        return;
    }

    cli_state.first_emu_launched = false;

    if (!cli_params.benchmark.empty())
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
    ++cli_state.dacrate_change_count;

    g_view_logger->trace("[Cli] on_dacrate_changed {}x", cli_state.dacrate_change_count);

    if (cli_state.is_movie_from_start && cli_state.dacrate_change_count == 2)
    {
        start_capture();
    }

    if (!cli_state.is_movie_from_start && cli_state.dacrate_change_count == 1)
    {
        start_capture();
    }
}

void CLI::init()
{
    Messenger::subscribe(Messenger::Message::CoreExecutingChanged, on_core_executing_changed);
    Messenger::subscribe(Messenger::Message::AppReady, on_app_ready);
    Messenger::subscribe(Messenger::Message::TaskChanged, on_task_changed);
    Messenger::subscribe(Messenger::Message::DacrateChanged, on_dacrate_changed);

    argh::parser cmdl(__argc, __argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);

    cli_params.rom = cmdl({"--rom", "-g"}, "").str();
    cli_params.lua = cmdl({"--lua", "-lua"}, "").str();
    cli_params.st = cmdl({"--st", "-st"}, "").str();
    cli_params.m64 = cmdl({"--movie", "-m64"}, "").str();
    cli_params.avi = cmdl({"--avi", "-avi"}, "").str();
    cli_params.benchmark = cmdl({"--benchmark", "-b"}, "").str();
    cli_params.close_on_movie_end = cmdl["--close-on-movie-end"];
    cli_params.wait_for_debugger = cmdl["--wait-for-debugger"] || cmdl["--d"];
    bool compare_control = cmdl["--cmp-ctl"] || cmdl["--compare-control"];
    bool compare_actual = cmdl["--cmp-act"] || cmdl["--compare-actual"];
    std::string compare_interval_str = cmdl({"--cmp-int", "--compare-interval"}, "100").str();
    size_t compare_interval = std::stoi(compare_interval_str);

    if (cli_params.wait_for_debugger)
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
        cli_params.rom = cmdl[1];
    }

    // COMPAT: Old mupen closes emu when movie ends and avi flag is specified.
    if (!cli_params.avi.empty())
    {
        cli_params.close_on_movie_end = true;
    }

    if (!cli_params.benchmark.empty() && cli_params.m64.empty() && cli_params.rom.empty())
    {
        DialogService::show_dialog(L"Benchmark flag specified without a movie.\nThe benchmark won't be performed.", L"CLI", fsvc_error);
        cli_params.benchmark.clear();
    }

    if (!cli_params.benchmark.empty() && !cli_params.avi.empty())
    {
        DialogService::show_dialog(L"Benchmark flag specified in combination with AVI.\nThe benchmark won't be performed.", L"CLI", fsvc_error);
        cli_params.benchmark.clear();
    }

    if (!cli_params.benchmark.empty())
    {
        cli_params.close_on_movie_end = true;
    }

    // If an st is specified, a movie mustn't be specified
    if (!cli_params.st.empty() && !cli_params.m64.empty())
    {
        DialogService::show_dialog(L"Both -st and -m64 options specified in CLI parameters.\nThe -st option will be dropped.", L"CLI", fsvc_error);
        cli_params.st.clear();
    }

    if (cli_params.close_on_movie_end && g_config.core.is_movie_loop_enabled)
    {
        DialogService::show_dialog(L"Movie loop is not allowed when closing on movie end is enabled.\nThe movie loop option will be disabled.", L"CLI", fsvc_warning);
        g_config.core.is_movie_loop_enabled = false;
    }
    
    // HACK: When playing a movie from start, the rom will start normally and signal us to do our work via EmuLaunchedChanged.
    // The work is started, but then the rom is reset. At that point, the dacrate changes and breaks the capture in some cases.
    // To avoid this, we store the movie's start flag prior to doing anything, and ignore the first EmuLaunchedChanged if it's set.
    const auto movie_path = cli_params.rom.extension() == ".m64" ? cli_params.rom : cli_params.m64;
    if (!movie_path.empty())
    {
        core_vcr_movie_header hdr{};
        core_vcr_parse_header(movie_path, &hdr);
        cli_state.is_movie_from_start = hdr.startFlags & MOVIE_START_FROM_NOTHING;
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

    cli_state.rom_is_movie = cli_params.rom.extension() == ".m64";

    log_cli_params(cli_params);
}

bool CLI::wants_fast_forward()
{
    return !cli_params.avi.empty() || !cli_params.benchmark.empty();
}
