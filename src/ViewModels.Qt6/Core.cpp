/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "ViewModels.Qt6/Core.hpp"
#include "ViewModels.Qt6/plugins/Plugin.hpp"
#include <CommonPCH.hpp>
#include <Common.Views/IDialogService.hpp>

#include <future>
#include <print>
#include <functional>

core_cfg &Core::config()
{
    static core_cfg s_instance{.core_type = 1};
    return s_instance;
}
core_params &Core::params()
{
    static core_params s_instance = [] -> core_params {
        auto params = core_params{
            .cfg = &config(),
            .callbacks =
                core_callbacks{
                    .emu_starting = [] { PluginUtil::start_plugins(Core::params()); },
                    .emu_stopped = PluginUtil::stop_plugins,
                },
            .log_trace = [](std::string_view msg) { std::println(stderr, "[TRACE] {}", msg); },
            .log_info = [](std::string_view msg) { std::println(stderr, "[INFO]  {}", msg); },
            .log_warn = [](std::string_view msg) { std::println(stderr, "[WARN]  {}", msg); },
            .log_error = [](std::string_view msg) { std::println(stderr, "[ERROR] {}", msg); },
            .load_plugins = PluginUtil::load_plugins,
            .initiate_plugins = [] { PluginUtil::initiate_plugins(Core::context(), Core::params()); },
            .submit_task =
                [](const auto &cb) {
                    // Defer to the stdlib's thread pool.
                    (void)std::async(cb);
                },
            .get_saves_directory =
                [] {
                    static auto s_save_path = IOUtils::exe_path().parent_path() / "saves";
                    if (!std::filesystem::is_directory(s_save_path)) std::filesystem::create_directories(s_save_path);
                    return s_save_path;
                },
            .get_backups_directory =
                [] {
                    static auto s_backups_path = IOUtils::exe_path().parent_path() / "backups";
                    if (!std::filesystem::is_directory(s_backups_path))
                        std::filesystem::create_directories(s_backups_path);
                    return s_backups_path;
                },
            .get_summercart_path = []() { return IOUtils::exe_path().parent_path() / "saves/cart.vhd"; },
            .show_multiple_choice_dialog =
                [](std::string_view id, const std::vector<std::string> &choices, const char *str, const char *title,
                   const core_dialog_type type) {
                    return DialogService::show_multiple_choice_dialog(
                        id, choices, str, title ? std::make_optional(title) : std::nullopt, type);
                },
            .show_ask_dialog =
                [](std::string_view id, const char *str, const char *title, const bool warning) {
                    return DialogService::show_ask_dialog(id, str, title ? std::make_optional(title) : std::nullopt,
                                                          warning);
                },
            .show_dialog =
                [](const char *str, const char *title, const core_dialog_type type) {
                    DialogService::show_dialog(str, title ? std::make_optional(title) : std::nullopt, type);
                },
            .get_plugin_names = PluginUtil::get_plugin_names,
        };
        Core::clear_plugin_funcs(params);
        return params;
    }();
    return s_instance;
}
core_ctx *Core::context()
{
    static core_ctx *s_pointer = [] -> core_ctx * {
        core_ctx *pointer = nullptr;
        if (core_create(&Core::params(), &pointer) != Res_Ok) throw std::logic_error("Failed to initialize core");

        return pointer;
    }();
    return s_pointer;
}

void Core::clear_plugin_funcs(core_params &params)
{
    params.video_process_dlist = [](auto...) {};
    params.video_process_rdp_list = [](auto...) {};
    params.video_show_cfb = [](auto...) {};
    params.video_vi_status_changed = [](auto...) {};
    params.video_vi_width_changed = [](auto...) {};
    params.video_get_video_size = [](auto...) {};
    params.video_fb_read = [](auto...) {};
    params.video_fb_write = [](auto...) {};
    params.video_fb_get_frame_buffer_info = [](auto...) {};
    params.audio_ai_dacrate_changed = [](auto...) {};
    params.audio_ai_len_changed = [](auto...) {};
    params.audio_ai_read_length = [](auto...) { return 0; };
    params.audio_process_alist = [](auto...) {};
    params.input_controller_command = [](auto...) {};
    params.input_get_keys = [](auto...) {};
    params.input_set_keys = [](auto...) {};
    params.input_read_controller = [](auto...) {};
    params.rsp_do_rsp_cycles = [](auto...) { return 0; };
}