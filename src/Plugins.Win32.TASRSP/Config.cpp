/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include "Config.hpp"

#define SUBKEY L"Software\\N64 Emulation\\DLL\\TAS RSP"
#define CONFIG_VALUE L"Config"

t_config config = {};
t_config default_config = {};
t_config prev_config = {};

static std::filesystem::path get_config_path()
{
    return g_config_path / CONFIG_FILE_NAME;
}

static void save_registry_config()
{
    g_ef->log_trace(L"Saving config...");

    HKEY h_key{};

    if (RegCreateKeyEx(HKEY_CURRENT_USER, SUBKEY, 0, NULL, 0, KEY_WRITE, NULL, &h_key, NULL) != ERROR_SUCCESS)
    {
        g_ef->log_error(L"RegCreateKeyEx failed");
        return;
    }

    if (RegSetValueEx(h_key, CONFIG_VALUE, 0, REG_BINARY, reinterpret_cast<const BYTE *>(&config), sizeof(t_config)) !=
        ERROR_SUCCESS)
    {
        g_ef->log_error(L"RegSetValueEx failed");
        RegCloseKey(h_key);
        return;
    }

    RegCloseKey(h_key);
}

void config_load()
{
    g_ef->log_trace(L"Loading config...");

    HKEY h_key{};
    DWORD size = sizeof(t_config);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, SUBKEY, 0, KEY_READ, &h_key) != ERROR_SUCCESS)
    {
        g_ef->log_error(L"RegCreateKeyEx failed");
        return;
    }

    t_config loaded_config{};

    if (RegQueryValueEx(h_key, CONFIG_VALUE, nullptr, nullptr, reinterpret_cast<BYTE *>(&loaded_config), &size) !=
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

    config = loaded_config;
}

void config_show_dialog(HWND hwnd)
{
    DialogBox(g_instance, MAKEINTRESOURCE(IDD_RSPCONFIG), hwnd, ConfigDlgProc);
}
