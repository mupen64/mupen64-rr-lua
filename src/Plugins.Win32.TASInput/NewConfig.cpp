/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <NewConfig.h>
#include <MiscHelpers.h>
#include <Main.h>

#define CONFIG_VALUE L"Config"

constexpr t_config default_config{};
t_config new_config{};

void save_config()
{
    g_ef->log_trace(L"Saving config...");

    HKEY h_key{};

    if (RegCreateKeyEx(HKEY_CURRENT_USER, SUBKEY, 0, NULL, 0, KEY_WRITE, NULL, &h_key, NULL) != ERROR_SUCCESS)
    {
        g_ef->log_error(L"RegCreateKeyEx failed");
        return;
    }

    if (RegSetValueEx(h_key, CONFIG_VALUE, 0, REG_BINARY, reinterpret_cast<const BYTE*>(&new_config), sizeof(t_config)) != ERROR_SUCCESS)
    {
        g_ef->log_error(L"RegSetValueEx failed");
        RegCloseKey(h_key);
        return;
    }

    RegCloseKey(h_key);
}

void load_config()
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

    if (RegQueryValueEx(
        h_key,
        CONFIG_VALUE,
        nullptr,
        nullptr,
        reinterpret_cast<BYTE*>(&loaded_config),
        &size) != ERROR_SUCCESS ||
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
