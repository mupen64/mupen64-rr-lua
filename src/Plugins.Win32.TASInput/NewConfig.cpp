/*
 * Copyright (c) 2026, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <NewConfig.hpp>
#include <MiscHelpers.hpp>
#include <Main.hpp>

const t_config default_config{};
t_config new_config{};

static std::filesystem::path get_config_path()
{
    return g_config_path / CONFIG_FILE_NAME;
}

void save_config()
{
    g_ef->log_trace(L"Saving config...");

    nlohmann::json j = new_config;
    std::ofstream ofs(get_config_path());
    ofs << std::setw(2) << j;
}

void load_config()
{
    g_ef->log_trace(L"Loading config...");

    auto json_path = get_config_path();

    if (!std::filesystem::exists(json_path)) {
        new_config = default_config;
        save_config();
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
        g_ef->log_warn(L"Config load failed, using defaults...");
        new_config = default_config;
    }
}
