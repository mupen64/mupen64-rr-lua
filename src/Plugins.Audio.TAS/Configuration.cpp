/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Configuration.h"
#include "resource.h"
#include "SoundDriverInterface.h"

#define SUBKEY "Software\\N64 Emulation\\DLL\\TAS Audio"
#define CONFIG_VALUE "Config"

extern HINSTANCE hInstance;
extern SoundDriverInterface *snd;
bool Configuration::RomRunning = false;
char Configuration::configAudioLogFolder[MAX_FOLDER_LENGTH];
t_trivial_config Configuration::currentSettings;
static t_trivial_config default_config{};
static t_trivial_config prev_config{};

bool Configuration::config_load()
{
    HKEY h_key{};
    DWORD size = sizeof(t_trivial_config);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, SUBKEY, 0, KEY_READ, &h_key) != ERROR_SUCCESS)
    {
        return false;
    }

    t_trivial_config loaded_config{};

    if (RegQueryValueEx(h_key, CONFIG_VALUE, nullptr, nullptr, reinterpret_cast<BYTE *>(&loaded_config), &size) !=
            ERROR_SUCCESS ||
        size != sizeof(t_trivial_config))
    {
        RegCloseKey(h_key);
        return false;
    }

    RegCloseKey(h_key);

    if (loaded_config.version < default_config.version)
    {
        loaded_config = default_config;
    }

    currentSettings = loaded_config;

    return true;
}

void Configuration::LoadSettings()
{
    config_load();
}

bool Configuration::config_save()
{
    HKEY h_key{};

    if (RegCreateKeyEx(HKEY_CURRENT_USER, SUBKEY, 0, NULL, 0, KEY_WRITE, NULL, &h_key, NULL) != ERROR_SUCCESS)
    {
        return false;
    }

    if (RegSetValueEx(h_key, CONFIG_VALUE, 0, REG_BINARY, reinterpret_cast<const BYTE *>(&currentSettings),
                      sizeof(t_trivial_config)) != ERROR_SUCCESS)
    {
        RegCloseKey(h_key);
        return false;
    }

    RegCloseKey(h_key);

    return true;
}

void Configuration::SaveSettings()
{
    config_save();
}

static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_INITDIALOG:
        Configuration::LoadSettings();

        memcpy(&prev_config, &Configuration::currentSettings, sizeof(t_trivial_config));

        SendMessage(GetDlgItem(hwnd, IDC_OLDSYNC), BM_SETCHECK,
                    Configuration::currentSettings.force_sync ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessage(GetDlgItem(hwnd, IDC_AI), BM_SETCHECK, Configuration::currentSettings.ai_emulation ? BST_CHECKED : BST_UNCHECKED,
                    0);
        SendMessage(GetDlgItem(hwnd, IDC_BUFFERS), TBM_SETTICFREQ, 1, 0);
        SendMessage(GetDlgItem(hwnd, IDC_BUFFERS), TBM_SETRANGEMIN, FALSE, 2);
        SendMessage(GetDlgItem(hwnd, IDC_BUFFERS), TBM_SETRANGEMAX, FALSE, 9);
        SendMessage(GetDlgItem(hwnd, IDC_BUFFERS), TBM_SETPOS, TRUE, Configuration::currentSettings.buffer_level);
        SendMessage(GetDlgItem(hwnd, IDC_SLIDER_BACKFPS), TBM_SETTICFREQ, 1, 0);
        SendMessage(GetDlgItem(hwnd, IDC_SLIDER_BACKFPS), TBM_SETRANGEMIN, FALSE, 1);
        SendMessage(GetDlgItem(hwnd, IDC_SLIDER_BACKFPS), TBM_SETRANGEMAX, FALSE, 8);
        SendMessage(GetDlgItem(hwnd, IDC_SLIDER_BACKFPS), TBM_SETPOS, TRUE, Configuration::currentSettings.backend_fps / 15);
        SendMessage(GetDlgItem(hwnd, IDC_SLIDER_BUFFERFPS), TBM_SETTICFREQ, 1, 0);
        SendMessage(GetDlgItem(hwnd, IDC_SLIDER_BUFFERFPS), TBM_SETRANGEMIN, FALSE, 1);
        SendMessage(GetDlgItem(hwnd, IDC_SLIDER_BUFFERFPS), TBM_SETRANGEMAX, FALSE, 8);
        SendMessage(GetDlgItem(hwnd, IDC_SLIDER_BUFFERFPS), TBM_SETPOS, TRUE, Configuration::currentSettings.buffer_fps / 15);
        SendMessage(GetDlgItem(hwnd, IDC_DISALLOWDS8), BM_SETCHECK,
                    Configuration::currentSettings.disallow_sleep_ds8 ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessage(GetDlgItem(hwnd, IDC_DISALLOWXA2), BM_SETCHECK,
                    Configuration::currentSettings.disallow_sleep_xa2 ? BST_CHECKED : BST_UNCHECKED, 0);
        char textPos[20];
        sprintf(textPos, "%li", Configuration::currentSettings.buffer_level);
        SetDlgItemText(hwnd, IDC_BUFFERS_TEXT, (LPCSTR)textPos);
        sprintf(textPos, "%li ms", 1000 / Configuration::currentSettings.backend_fps);
        SetDlgItemText(hwnd, IDC_SLIDER_BACKFPS_TEXT, (LPCSTR)textPos);
        sprintf(textPos, "%li ms", 1000 / Configuration::currentSettings.buffer_fps);
        SetDlgItemText(hwnd, IDC_SLIDER_BUFFERFPS_TEXT, (LPCSTR)textPos);

        SendMessage(GetDlgItem(hwnd, IDC_DEVICE), CB_RESETCONTENT, 0, 0);
        SendMessage(GetDlgItem(hwnd, IDC_DEVICE), CB_ADDSTRING, 0, (LPARAM)(char *)"Default");
        SendMessage(GetDlgItem(hwnd, IDC_DEVICE), CB_SETCURSEL, 0, 0);
        SendMessage(GetDlgItem(hwnd, IDC_VOLUME), TBM_SETPOS, TRUE, Configuration::currentSettings.volume);
        SendMessage(GetDlgItem(hwnd, IDC_VOLUME), TBM_SETTICFREQ, 20, 0);
        SendMessage(GetDlgItem(hwnd, IDC_VOLUME), TBM_SETRANGEMIN, FALSE, 0);
        SendMessage(GetDlgItem(hwnd, IDC_VOLUME), TBM_SETRANGEMAX, FALSE, 100);
        if (Configuration::currentSettings.volume == 100)
        {
            SendMessage(GetDlgItem(hwnd, IDC_MUTE), BM_SETCHECK, BST_CHECKED, 0);
        }
        else
        {
            SendMessage(GetDlgItem(hwnd, IDC_MUTE), BM_SETCHECK, BST_UNCHECKED, 0);
        }

        break;
    case WM_CLOSE:
        Configuration::SaveSettings();
        EndDialog(hwnd, IDOK);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:

            Configuration::setForceSync(
                SendMessage(GetDlgItem(hwnd, IDC_OLDSYNC), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);
            Configuration::setAIEmulation(
                SendMessage(GetDlgItem(hwnd, IDC_AI), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);
            Configuration::setBufferLevel((unsigned long)SendMessage(GetDlgItem(hwnd, IDC_BUFFERS), TBM_GETPOS, 0, 0));
            Configuration::setBackendFPS(
                (unsigned long)SendMessage(GetDlgItem(hwnd, IDC_SLIDER_BACKFPS), TBM_GETPOS, 0, 0) * 15);
            Configuration::setBufferFPS(
                (unsigned long)SendMessage(GetDlgItem(hwnd, IDC_SLIDER_BUFFERFPS), TBM_GETPOS, 0, 0) * 15);
            Configuration::setDisallowSleepDS8(
                SendMessage(GetDlgItem(hwnd, IDC_DISALLOWDS8), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);
            Configuration::setDisallowSleepXA2(
                SendMessage(GetDlgItem(hwnd, IDC_DISALLOWXA2), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);

            Configuration::currentSettings.volume =
                (unsigned long)SendMessage(GetDlgItem(hwnd, IDC_VOLUME), TBM_GETPOS, 0, 0);
            Configuration::setVolume(Configuration::currentSettings.volume);
            snd->SetVolume(Configuration::currentSettings.volume);

            Configuration::SaveSettings();

            EndDialog(hwnd, IDOK);
            break;
        case IDCANCEL:
            memcpy(&Configuration::currentSettings, &prev_config, sizeof(t_trivial_config));
            Configuration::SaveSettings();

            EndDialog(hwnd, IDCANCEL);
            break;
        case IDC_MUTE:
            if (IsDlgButtonChecked(hwnd, IDC_MUTE))
            {
                SendMessage(GetDlgItem(hwnd, IDC_VOLUME), TBM_SETPOS, TRUE, 100);
                snd->SetVolume(100);
            }
            else
            {
                SendMessage(GetDlgItem(hwnd, IDC_VOLUME), TBM_SETPOS, TRUE, Configuration::currentSettings.volume);
                snd->SetVolume(Configuration::currentSettings.volume);
            }
            break;

        default:
            break;
        }
        break;
    case WM_HSCROLL:
        if (lParam != 0)
        {
            char textPos[20];
            unsigned long dwPosition;
            switch (GetDlgCtrlID((HWND)lParam))
            {
            case IDC_BUFFERS:
                dwPosition = (unsigned long)SendMessage(GetDlgItem(hwnd, IDC_BUFFERS), TBM_GETPOS, 0, 0);
                sprintf(textPos, "%li", dwPosition);
                SetDlgItemText(hwnd, IDC_BUFFERS_TEXT, (LPCSTR)textPos);
                break;
            case IDC_SLIDER_BACKFPS:
                dwPosition = (unsigned long)SendMessage(GetDlgItem(hwnd, IDC_SLIDER_BACKFPS), TBM_GETPOS, 0, 0);
                sprintf(textPos, "%li ms", (DWORD)(1000 / (dwPosition * 15)));
                SetDlgItemText(hwnd, IDC_SLIDER_BACKFPS_TEXT, (LPCSTR)textPos);
                break;
            case IDC_SLIDER_BUFFERFPS:
                dwPosition = (unsigned long)SendMessage(GetDlgItem(hwnd, IDC_SLIDER_BUFFERFPS), TBM_GETPOS, 0, 0);
                sprintf(textPos, "%li ms", (DWORD)(1000 / (dwPosition * 15)));
                SetDlgItemText(hwnd, IDC_SLIDER_BUFFERFPS_TEXT, (LPCSTR)textPos);
                break;
            }
        }
        break;
    case WM_VSCROLL: {
        short int userReq = LOWORD(wParam);
        if (userReq == TB_ENDTRACK || userReq == TB_THUMBTRACK)
        {
            unsigned long dwPosition = (unsigned long)SendMessage(GetDlgItem(hwnd, IDC_VOLUME), TBM_GETPOS, 0, 0);
            if (dwPosition == 100)
            {
                SendMessage(GetDlgItem(hwnd, IDC_MUTE), BM_SETCHECK, BST_CHECKED, 0);
            }
            else
            {
                SendMessage(GetDlgItem(hwnd, IDC_MUTE), BM_SETCHECK, BST_UNCHECKED, 0);
            }

            Configuration::setVolume(dwPosition);
            snd->SetVolume(dwPosition);
        }
        break;
    }
    default:
        break;
    }
    return FALSE;
}

void Configuration::ConfigDialog(HWND hParent)
{
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_CONFIG), hParent, DlgProc);
}
