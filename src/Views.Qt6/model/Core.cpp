/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Core.hpp"

#include <cassert>
#include <exception>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <utility>

#include <boost/dll/runtime_symbol_info.hpp>
#include <spdlog/common.h>
#include <spdlog/logger.h>

#include "core_api.h"
#include "core_types.h"

#include "Plugin.hpp"
#include "Logging.hpp"

namespace
{

std::mutex &core_state_mutex()
{
    static std::mutex mutex;
    return mutex;
}

} // namespace

namespace Mupen
{
core_cfg g_core_cfg{};
core_params g_core_params{};
core_ctx *g_core_ctx = nullptr;

static std::optional<std::filesystem::path> g_rom_path = std::nullopt;
static std::optional<PluginPaths> g_plugin_paths = std::nullopt;
static std::optional<PluginSet> g_curr_plugins = std::nullopt;
static std::unique_ptr<ICoreService> g_core_service = nullptr;

void core_log(spdlog::level::level_enum level, std::string_view message)
{
    Mupen::core_log().log(level, message);
}

void core_init(core_cfg config, std::unique_ptr<ICoreService> &&core_service)
{
    {
        std::scoped_lock _lock(core_state_mutex());
        g_core_cfg = std::move(config);
        g_core_service = std::move(core_service);
    }

    g_core_params = core_params{
        .cfg = &g_core_cfg,
        .callbacks =
            {
                .emu_stopped =
                    []() {
                        std::scoped_lock _lock(core_state_mutex());
                        g_rom_path = std::nullopt;
                        g_plugin_paths = std::nullopt;
                        g_curr_plugins = std::nullopt;
                    },
            },
        .controls = {},

        // CORE HOOKS
        // =====================
        .log_trace = [](std::string_view str) { Mupen::core_log().trace(str); },
        .log_info = [](std::string_view str) { Mupen::core_log().info(str); },
        .log_warn = [](std::string_view str) { Mupen::core_log().warn(str); },
        .log_error = [](std::string_view str) { Mupen::core_log().error(str); },

        .load_plugins =
            []() {
                assert(!g_curr_plugins.has_value());

                if (!g_plugin_paths.has_value())
                {
                    core_log().error("Plugin paths missing!");
                    return false;
                }
                try
                {
                    auto plugins = PluginSet{
                        g_plugin_paths->video_path,
                        g_plugin_paths->audio_path,
                        g_plugin_paths->input_path,
                        g_plugin_paths->rsp_path,
                    };
                    g_curr_plugins.emplace(std::move(plugins));
                    return true;
                }
                catch (const std::exception &except)
                {
                    core_log().error(except.what());
                    return false;
                }
            },
        .initiate_plugins =
            []() {
                assert(g_curr_plugins.has_value());
                assert(g_core_service.get() != nullptr);
                g_curr_plugins->resolve_functions_to(g_core_params);
                g_curr_plugins->initiate_all(*g_core_ctx, g_core_params, *g_core_service);
            },
        .submit_task =
            [](const std::function<void()> &fn) {
                // this is entirely a set-and-forget thing.
                std::ignore = std::async(fn);
            },
        .get_saves_directory =
            []() {
                auto boost_dir = boost::dll::program_location();
                auto saves_dir = std::filesystem::path(boost_dir.c_str()).parent_path() / "saves";
                if (!std::filesystem::is_directory(saves_dir)) std::filesystem::create_directory(saves_dir);
                return saves_dir;
            },
        .get_backups_directory =
            []() {
                auto boost_dir = boost::dll::program_location();
                auto saves_dir = std::filesystem::path(boost_dir.c_str()).parent_path() / "backups";
                if (!std::filesystem::is_directory(saves_dir)) std::filesystem::create_directory(saves_dir);
                return saves_dir;
            },
        .show_multiple_choice_dialog =
            [](std::string_view id, const std::vector<std::string> &choices, const char *str, const char *title,
               core_dialog_type type) { return g_core_service->show_choice_dialog(id, choices, title, str, type); },
        .show_ask_dialog =
            [](std::string_view id, const char *str, const char *title, bool warning) {
                using namespace std::literals;
                static const std::array yes_no_choices = std::array{"Yes"s, "No"s};

                auto type = warning ? fsvc_warning : fsvc_information;
                auto res = g_core_service->show_choice_dialog(id, yes_no_choices, title, str, type) == 0;
                return false;
            },
        .show_dialog = [](const char *str, const char *title,
                          core_dialog_type type) { g_core_service->show_info_dialog(title, str, type); },
        .update_screen = []() {},
        .copy_video = [](void*) {},
        .find_available_rom =
            [](const std::function<bool(const core_rom_header &)> &predicate) { return std::filesystem::path(""); },
        .mge_available = []() { return false; },
        .load_screen = [](void *data) {},
        .get_plugin_names =
            [](char *video, char *audio, char *input, char *rsp) {
                assert(g_curr_plugins.has_value());
                g_curr_plugins->extract_names(video, audio, input, rsp);
            },
    };

    auto res = ::core_create(&g_core_params, &g_core_ctx);
    if (res != Res_Ok)
    {
        core_log().critical("Core failed to load! ({})", (int)res);
    }
}

void core_start(const std::filesystem::path &rom_path, const PluginPaths &plugin_paths)
{
    {
        std::scoped_lock _lock(core_state_mutex());
        g_rom_path = rom_path;
        g_plugin_paths = plugin_paths;
    }

    g_core_ctx->vr_start_rom(rom_path);
}

void core_stop()
{
    g_core_ctx->vr_close_rom(true);
}
} // namespace Mupen