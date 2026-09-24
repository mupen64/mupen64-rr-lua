/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include "Config.hpp"

#define CONFIG_FILE_NAME "TASRSP.json"

RSPConfig config = {};
RSPConfig default_config = {};
RSPConfig prev_config = {};

static std::filesystem::path get_config_path()
{
    const auto size = g_plugin->config_path(nullptr, 0);
    std::string path(size - 1, '\0');
    g_plugin->config_path(path.data(), size);
    return std::filesystem::path(path) / CONFIG_FILE_NAME;
}

void config_save()
{
    g_plugin->log_trace("Saving config...");

    nlohmann::json j = config;
    std::ofstream ofs(get_config_path());
    ofs << std::setw(2) << j;
}

void config_load()
{
    g_plugin->log_trace("Loading config...");

    auto json_path = get_config_path();

    if (!std::filesystem::exists(json_path))
    {
        config = default_config;
        config_save();
        return;
    }

    std::ifstream ifs(json_path);
    nlohmann::json j;

    try
    {
        ifs >> j;
        nlohmann::from_json(j, config);
    }
    catch (const std::exception &e)
    {
        g_plugin->log_warn("Config load failed, using defaults...");
        config = default_config;
    }
}
