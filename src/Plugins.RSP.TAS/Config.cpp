/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.h"
#include "Config.h"

#define CONFIG_PATH "hacktarux-azimer-rsp-hle.cfg"

t_config config = {};
t_config default_config = {};
t_config prev_config = {};

INT_PTR CALLBACK ConfigDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_INITDIALOG:
        config_load();
        memcpy(&prev_config, &config, sizeof(t_config));
        CheckDlgButton(hwnd, IDC_UCODE_CACHE_VERIFY, config.ucode_cache_verify ? BST_CHECKED : BST_UNCHECKED);
        break;
    case WM_CLOSE:
        config_save();
        EndDialog(hwnd, IDOK);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            config.ucode_cache_verify = IsDlgButtonChecked(hwnd, IDC_UCODE_CACHE_VERIFY);
            config_save();
            EndDialog(hwnd, IDOK);
            break;
        case IDCANCEL:
            memcpy(&config, &prev_config, sizeof(t_config));
            config_save();
            EndDialog(hwnd, IDCANCEL);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

void config_save()
{
    printf("[RSP] Saving config...\n");
    FILE *f = fopen(CONFIG_PATH, "wb");
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
    
    auto buffer = IOUtils::read_entire_file(CONFIG_PATH);
    t_config loaded_config;

    if (buffer.empty() || buffer.size() != sizeof(t_config))
    {
        // Failed, reset to default
        printf("[RSP] No config found, using default\n");
        memcpy(&loaded_config, &default_config, sizeof(t_config));
    }
    else
    {
        uint8_t *ptr = buffer.data();
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

void config_show_dialog(HWND hwnd)
{
    DialogBox(g_instance, MAKEINTRESOURCE(IDD_RSPCONFIG), hwnd, ConfigDlgProc);
}
