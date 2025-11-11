/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Core.hpp"
#include "core_api.h"
#include <optional>
#include <spdlog/logger.h>

namespace
{

std::optional<std::filesystem::path> g_video_plugin_path;
std::optional<std::filesystem::path> g_audio_plugin_path;
std::optional<std::filesystem::path> g_input_plugin_path;
std::optional<std::filesystem::path> g_rsp_plugin_path;

// core logger
spdlog::logger& core_logger() {
    static spdlog::logger logger("mupen64");
    return logger;
}

} // namespace

namespace Mupen
{
    core_cfg g_core_cfg {};
    core_params g_core_params {};
    core_ctx* g_core_ctx = nullptr;

void core_init(core_cfg config) {
    g_core_cfg = config;
    g_core_params = {
        .cfg = &g_core_cfg,
        .callbacks = {},
        .controls = {},

        // CORE HOOKS
        // =====================
        .log_trace = [](const std::string& str) {
            core_logger().trace(str);
        },
        .log_info = [](const std::string& str) {
            core_logger().info(str);
        },
        .log_warn = [](const std::string& str) {
            core_logger().warn(str);
        },
        .log_error = [](const std::string& str) {
            core_logger().error(str);
        },
    };
}
}