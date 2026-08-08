/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include "Dialog.hpp"
#include "Plugin.hpp"
#include <future>
#include <iostream>
#include <print>

core_cfg g_config;
core_params g_core_params{};
core_ctx *g_core_ctx = nullptr;

void clear_plugin_funcs()
{
    g_core_params.video_process_dlist = [](auto...) {};
    g_core_params.video_process_rdp_list = [](auto...) {};
    g_core_params.video_show_cfb = [](auto...) {};
    g_core_params.video_vi_status_changed = [](auto...) {};
    g_core_params.video_vi_width_changed = [](auto...) {};
    g_core_params.video_get_video_size = [](auto...) {};
    g_core_params.video_fb_read = [](auto...) {};
    g_core_params.video_fb_write = [](auto...) {};
    g_core_params.video_fb_get_frame_buffer_info = [](auto...) {};
    g_core_params.audio_ai_dacrate_changed = [](auto...) {};
    g_core_params.audio_ai_len_changed = [](auto...) {};
    g_core_params.audio_ai_read_length = [](auto...) { return 0; };
    g_core_params.audio_process_alist = [](auto...) {};
    g_core_params.input_controller_command = [](auto...) {};
    g_core_params.input_get_keys = [](auto...) {};
    g_core_params.input_set_keys = [](auto...) {};
    g_core_params.input_read_controller = [](auto...) {};
    g_core_params.rsp_do_rsp_cycles = [](auto...) { return 0; };
}

static void init_core()
{
    g_core_params.cfg = &g_config;
    clear_plugin_funcs();

    // SETTINGS
    // =====================================================
    g_config.core_type = 2;

    // EXTRA CALLBACKS
    // =====================================================

    g_core_params.callbacks.emu_starting = PluginUtil::start_plugins;
    g_core_params.callbacks.emu_stopped = PluginUtil::stop_plugins;

    // MAIN CORE CALLBACKS
    // =====================================================

    g_core_params.log_error = [](std::string_view msg) { std::println(stderr, "[ERROR] {}", msg); };
    g_core_params.log_warn = [](std::string_view msg) { std::println(stderr, "[WARN]  {}", msg); };
    g_core_params.log_info = [](std::string_view msg) { std::println(stderr, "[INFO]  {}", msg); };
    g_core_params.log_trace = [](std::string_view msg) { std::println(stderr, "[TRACE] {}", msg); };

    g_core_params.load_plugins = PluginUtil::load_plugins;
    g_core_params.initiate_plugins = PluginUtil::initiate_plugins;
    g_core_params.submit_task = [](const auto &cb) {
        // Defer to the stdlib's thread pool.
        (void)std::async(cb);
    };
    g_core_params.get_saves_directory = []() {
        static auto s_save_path = IOUtils::exe_path().parent_path() / "saves";
        if (!std::filesystem::is_directory(s_save_path)) std::filesystem::create_directories(s_save_path);
        return s_save_path;
    };
    g_core_params.get_backups_directory = []() {
        static auto s_backups_path = IOUtils::exe_path().parent_path() / "backups";
        if (!std::filesystem::is_directory(s_backups_path)) std::filesystem::create_directories(s_backups_path);
        return s_backups_path;
    };
    g_core_params.get_summercart_path = []() { return IOUtils::exe_path().parent_path() / "saves/cart.vhd"; };
    g_core_params.show_multiple_choice_dialog = g_dialog_service->show_multiple_choice_dialog;
    g_core_params.show_ask_dialog = g_dialog_service->show_ask_dialog;
    g_core_params.show_dialog = g_dialog_service->show_dialog;
    g_core_params.get_plugin_names = PluginUtil::get_plugin_names;

    core_create(&g_core_params, &g_core_ctx);
}

int main(int argc, char *argv[])
{
    using namespace std::literals;
    if (argc != 2)
    {
        std::println("usage: {} [path to ROM]", argv[0]);
        return 1;
    }

    init_core();
    core_result res1 = g_core_ctx->vr_start_rom(argv[1]);
    std::println("result: {}", (int)res1);
    std::this_thread::sleep_for(10s);
    g_core_ctx->vr_close_rom(true);
}
