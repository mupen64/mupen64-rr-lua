/*
 * Copyright (c) 2026, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.h"
#include <NewConfig.h>
#include <MiscHelpers.h>
#include <Main.h>

#define REG_SUBKEY L"Software\\N64 Emulation\\DLL\\TASDI"
#define REG_CONFIG_VALUE L"Config"

const t_config default_config{};
t_config new_config{};

static std::filesystem::path get_config_path()
{
    return g_config_path / CONFIG_FILE_NAME;
}

static void save_registry_config()
{
    g_ef->log_trace(L"Saving config...");

    HKEY h_key{};

    if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_SUBKEY, 0, NULL, 0, KEY_WRITE, NULL, &h_key, NULL) != ERROR_SUCCESS)
    {
        g_ef->log_error(L"RegCreateKeyEx failed");
        return;
    }

    if (RegSetValueEx(h_key, REG_CONFIG_VALUE, 0, REG_BINARY, reinterpret_cast<const BYTE *>(&new_config),
                      sizeof(t_config)) != ERROR_SUCCESS)
    {
        g_ef->log_error(L"RegSetValueEx failed");
        RegCloseKey(h_key);
        return;
    }

    RegCloseKey(h_key);
}

static void load_registry_config()
{
    g_ef->log_trace(L"Loading config...");

    HKEY h_key{};
    DWORD size = sizeof(t_config);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_SUBKEY, 0, KEY_READ, &h_key) != ERROR_SUCCESS)
    {
        g_ef->log_error(L"RegCreateKeyEx failed");
        return;
    }

    t_config loaded_config{};

    if (RegQueryValueEx(h_key, REG_CONFIG_VALUE, nullptr, nullptr, reinterpret_cast<BYTE *>(&loaded_config), &size) !=
            ERROR_SUCCESS ||
        size != sizeof(t_config))
    {
        g_ef->log_error(L"RegQueryValueEx failed");
        RegCloseKey(h_key);
        return;
    }

    RegCloseKey(h_key);

    if (loaded_config.version < default_config.version)
    {
        g_ef->log_trace(L"Outdated config version, using default");
        loaded_config = default_config;
    }

    new_config = loaded_config;
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

    if (std::filesystem::exists(json_path))
    {
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
    else
    {
        g_ef->log_warn(L"No JSON config was present, attempting to load from registry");
        load_registry_config();

        save_config();
    }
}
