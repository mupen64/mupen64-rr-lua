/*
 * Copyright (c) 2025, hacktarux-azimer-rsp-hle maintainers, contributors, and original authors (Hacktarux, Azimer).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "win.h"
#include "Config.h"

#define CONFIG_PATH "hacktarux-azimer-rsp-hle.cfg"

t_config config = {};
t_config default_config = {};
const wchar_t* cfg_groups[] = {L"General"};
int32_t g_config_readonly{};

void config_save()
{
    printf("[RSP] Saving config...\n");
    FILE* f = fopen(CONFIG_PATH, "wb");
    if (!f)
    {
        printf("[RSP] Can't save config\n");
        return;
    }
    fwrite(&config, sizeof(t_config), 1, f);
    fclose(f);
}

void config_load()
{
    printf("[RSP] Loading config...\n");

    auto buffer = g_platform_service.read_file_buffer(CONFIG_PATH);
    t_config loaded_config;

    if (buffer.empty() || buffer.size() != sizeof(t_config))
    {
        // Failed, reset to default
        printf("[RSP] No config found, using default\n");
        memcpy(&loaded_config, &default_config, sizeof(t_config));
    }
    else
    {
        uint8_t* ptr = buffer.data();
        MiscHelpers::memread(&ptr, &loaded_config, sizeof(t_config));
    }

    if (loaded_config.version < default_config.version)
    {
        // Outdated version, reset to default
        printf("[RSP] Outdated config version, using default\n");
        memcpy(&loaded_config, &default_config, sizeof(t_config));
    }

    memcpy(&config, &loaded_config, sizeof(t_config));
}
