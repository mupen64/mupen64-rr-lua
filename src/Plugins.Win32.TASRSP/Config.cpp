/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include "Config.hpp"

#define CONFIG_FILE_NAME "TASRSP.json"

t_config config = {};
t_config default_config = {};
t_config prev_config = {};

static std::filesystem::path get_config_path()
{
    return g_config_path / CONFIG_FILE_NAME;
}

void config_save()
{
    g_plugin->log_trace(L"Saving config...");

    nlohmann::json j = config;
    std::ofstream ofs(get_config_path());
    ofs << std::setw(2) << j;
}

void config_load()
{
    g_plugin->log_trace(L"Loading config...");

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
        g_plugin->log_warn(L"Config load failed, using defaults...");
        config = default_config;
    }
}
