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

        if (!config.audio_hle && !config.audio_external)
        {
            CheckDlgButton(hwnd, IDC_ALISTS_INSIDE_RSP, BST_CHECKED);
        }
        if (config.audio_hle && !config.audio_external)
        {
            CheckDlgButton(hwnd, IDC_ALISTS_EMU_DEFINED_PLUGIN, BST_CHECKED);
        }
        if (config.audio_hle && config.audio_external)
        {
            CheckDlgButton(hwnd, IDC_ALISTS_RSP_DEFINED_PLUGIN, BST_CHECKED);
        }
        CheckDlgButton(hwnd, IDC_UCODE_CACHE_VERIFY, config.ucode_cache_verify ? BST_CHECKED : BST_UNCHECKED);
        goto refresh;
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
        case IDC_ALISTS_INSIDE_RSP:
            config.audio_hle = FALSE;
            config.audio_external = FALSE;
            goto refresh;
        case IDC_ALISTS_EMU_DEFINED_PLUGIN:
            config.audio_hle = TRUE;
            config.audio_external = FALSE;
            goto refresh;
        case IDC_ALISTS_RSP_DEFINED_PLUGIN:
            config.audio_hle = TRUE;
            config.audio_external = TRUE;
            goto refresh;
        case IDC_BROWSE_AUDIO_PLUGIN:
            MessageBox(hwnd,
                       L"Warning: use this feature at your own risk\n"
                       "It allows you to use a second audio plugin to process alists\n"
                       "before they are sent\n"
                       "Example: jabo's sound plugin in emu config to output sound\n"
                       "        +azimer's sound plugin in rsp config to process alists\n"
                       "Do not choose the same plugin in both place, or it'll probably crash\n"
                       "For more informations, please read the README",
                       L"Warning",
                       MB_OK);

            wchar_t path[std::size(config.audio_path)] = {0};
            OPENFILENAME ofn{};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = path;
            ofn.nMaxFile = sizeof(path);
            ofn.lpstrFilter = L"DLL Files (*.dll)\0*.dll";
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

            if (GetOpenFileName(&ofn))
            {
                lstrcpynW(config.audio_path, path, std::size(config.audio_path));
            }

            goto refresh;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;

refresh:
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_AUDIO_PLUGIN), config.audio_external);
    EnableWindow(GetDlgItem(hwnd, IDC_BROWSE_AUDIO_PLUGIN), config.audio_external);
    SetDlgItemText(hwnd, IDC_EDIT_AUDIO_PLUGIN, config.audio_path);

    return TRUE;
}

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


void config_show_dialog(HWND hwnd)
{
    DialogBox(g_instance, MAKEINTRESOURCE(IDD_RSPCONFIG), hwnd, ConfigDlgProc);
}
