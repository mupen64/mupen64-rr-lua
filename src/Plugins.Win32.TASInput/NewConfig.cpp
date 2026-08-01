/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <NewConfig.hpp>
#include <MiscHelpers.hpp>
#include <Main.hpp>
#include <GamepadManager.hpp>

const t_config default_config{};
t_config new_config{};

static std::filesystem::path get_config_path()
{
    const auto size = g_plugin->config_path(nullptr, 0);
    std::string path(size - 1, '\0');
    g_plugin->config_path(path.data(), size);
    return std::filesystem::path(path) / CONFIG_FILE_NAME;
}

void save_config()
{
    g_plugin->log_trace(L"Saving config...");

    nlohmann::json j = new_config;
    std::ofstream ofs(get_config_path());
    ofs << std::setw(2) << j;

    GamepadManager::update_current_gamepad();
}

void load_config()
{
    g_plugin->log_trace(L"Loading config...");

    auto json_path = get_config_path();

    if (!std::filesystem::exists(json_path))
    {
        new_config = default_config;
        save_config();
        GamepadManager::update_current_gamepad();
        return;
    }

    std::ifstream ifs(json_path);
    nlohmann::json j;

    try
    {
        ifs >> j;
        nlohmann::from_json(j, new_config);
    }
    catch (const std::exception &e)
    {
        g_plugin->log_warn(L"Config load failed, using defaults...");
        new_config = default_config;
    }

    GamepadManager::update_current_gamepad();
}
