/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Core.hpp"
#include "Plugin.hpp"
#include "core_api.h"
#include "mupapi.h"
#include <cassert>
#include <exception>
#include <functional>
#include <future>
#include <optional>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <tuple>
#include <utility>

namespace
{

// core logger
spdlog::logger &core_logger()
{
    static spdlog::logger logger("mupen64");
    return logger;
}

} // namespace

namespace Mupen
{
core_cfg g_core_cfg{};
core_params g_core_params{};
core_ctx *g_core_ctx = nullptr;

static std::optional<PluginPaths> g_plugin_paths = std::nullopt;
static std::optional<PluginSet> g_curr_plugins = std::nullopt;
static std::optional<std::function<mup_wm_handle(const mupv_wm_settings &)>> g_create_window = std::nullopt;

void core_log(spdlog::level::level_enum level, std::string_view message)
{
    core_logger().log(level, message);
}

void core_init(core_cfg config, const std::function<mup_wm_handle(const mupv_wm_settings &)> &create_window)
{
    g_core_cfg = std::move(config);
    g_create_window = create_window;

    g_core_params = core_params{
        .cfg = &g_core_cfg,
        .callbacks = {.emu_stopped = []() {}},
        .controls = {},

        // CORE HOOKS
        // =====================
        .log_trace = [](const std::string &str) { core_logger().trace(str); },
        .log_info = [](const std::string &str) { core_logger().info(str); },
        .log_warn = [](const std::string &str) { core_logger().warn(str); },
        .log_error = [](const std::string &str) { core_logger().error(str); },

        .load_plugins =
            []() {
                assert(g_curr_plugins.has_value());

                auto core_functions = mup_core_functions{
                    .size = sizeof(mup_core_functions),
                    .log_trace = [](const char *x) { core_log(spdlog::level::trace, x); },
                    .log_info = [](const char *x) { core_log(spdlog::level::info, x); },
                    .log_warn = [](const char *x) { core_log(spdlog::level::warn, x); },
                    .log_error = [](const char *x) { core_log(spdlog::level::err, x); },
                };

                if (!g_plugin_paths.has_value())
                {
                    core_logger().error("Plugin paths missing!");
                    return false;
                }
                try
                {
                    auto plugins = PluginSet{
                        core_functions,
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
                    core_logger().error(except.what());
                    return false;
                }
            },
        .initiate_plugins =
            []() {
                assert(g_curr_plugins.has_value());
                assert(g_create_window.has_value());
                g_curr_plugins->resolve_functions_to(g_core_params);
                g_curr_plugins->initiate_all(*g_core_ctx, g_core_params, *g_create_window);
            },
        .submit_task =
            [](const std::function<void()> &fn) {
                // this is entirely a set-and-forget thing.
                std::ignore = std::async(fn);
            },
    };
}
} // namespace Mupen