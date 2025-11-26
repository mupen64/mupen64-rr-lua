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
unsigned long Configuration::configVolume;
char Configuration::configAudioLogFolder[MAX_FOLDER_LENGTH];
t_trivial_config Configuration::currentSettings;

INT_PTR CALLBACK ConfigProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AdvancedProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

static t_trivial_config default_config{};

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
    if (snd && RomRunning) snd->SetVolume(configVolume);
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

void Configuration::ConfigDialog(HWND hParent)
{
    LoadSettings();

    PROPSHEETHEADER psh;
    PROPSHEETPAGE psp[2];

    memset(psp, 0, sizeof(psp));
    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_GENERAL);
    psp[0].pfnDlgProc = SettingsProc;
    psp[0].pszTitle = "Settings";
    psp[0].lParam = 0;
    psp[0].pfnCallback = NULL;

    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = hInstance;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_ADVANCED);
    psp[1].pfnDlgProc = AdvancedProc;
    psp[1].pszTitle = "Advanced";
    psp[1].lParam = 0;
    psp[1].pfnCallback = NULL;

    memset(&psh, 0, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hParent;
    psh.hInstance = hInstance;
    psh.pszCaption = "Audio Options";
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE)&psp;
    psh.pfnCallback = NULL;

    PropertySheet(&psh);

    SaveSettings();
}

void Configuration::ResetAdvancedPage(HWND hDlg)
{
    t_trivial_config tmp = currentSettings;
    SendMessage(GetDlgItem(hDlg, IDC_OLDSYNC), BM_SETCHECK, tmp.force_sync ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hDlg, IDC_AUDIOSYNC), BM_SETCHECK, tmp.sync_audio ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hDlg, IDC_AI), BM_SETCHECK, tmp.ai_emulation ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hDlg, IDC_BUFFERS), TBM_SETTICFREQ, 1, 0);
    SendMessage(GetDlgItem(hDlg, IDC_BUFFERS), TBM_SETRANGEMIN, FALSE, 2);
    SendMessage(GetDlgItem(hDlg, IDC_BUFFERS), TBM_SETRANGEMAX, FALSE, 9);
    SendMessage(GetDlgItem(hDlg, IDC_BUFFERS), TBM_SETPOS, TRUE, tmp.buffer_level);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BACKFPS), TBM_SETTICFREQ, 1, 0);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BACKFPS), TBM_SETRANGEMIN, FALSE, 1);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BACKFPS), TBM_SETRANGEMAX, FALSE, 8);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BACKFPS), TBM_SETPOS, TRUE, tmp.backend_fps / 15);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BUFFERFPS), TBM_SETTICFREQ, 1, 0);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BUFFERFPS), TBM_SETRANGEMIN, FALSE, 1);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BUFFERFPS), TBM_SETRANGEMAX, FALSE, 8);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BUFFERFPS), TBM_SETPOS, TRUE, tmp.buffer_fps / 15);
    SendMessage(GetDlgItem(hDlg, IDC_DISALLOWDS8), BM_SETCHECK, tmp.disallow_sleep_ds8 ? BST_CHECKED : BST_UNCHECKED,
                0);
    SendMessage(GetDlgItem(hDlg, IDC_DISALLOWXA2), BM_SETCHECK, tmp.disallow_sleep_xa2 ? BST_CHECKED : BST_UNCHECKED,
                0);
    char textPos[20];
    sprintf(textPos, "%li", tmp.buffer_level);
    SetDlgItemText(hDlg, IDC_BUFFERS_TEXT, (LPCSTR)textPos);
    sprintf(textPos, "%li ms", 1000 / tmp.backend_fps);
    SetDlgItemText(hDlg, IDC_SLIDER_BACKFPS_TEXT, (LPCSTR)textPos);
    sprintf(textPos, "%li ms", 1000 / tmp.buffer_fps);
    SetDlgItemText(hDlg, IDC_SLIDER_BUFFERFPS_TEXT, (LPCSTR)textPos);
}

INT_PTR CALLBACK Configuration::AdvancedProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG: {
        SendMessage(GetDlgItem(hDlg, IDC_PROFILE), CB_RESETCONTENT, 0, 0);
        ResetAdvancedPage(hDlg);
    }
    break;
    case WM_NOTIFY:
        if (((NMHDR FAR *)lParam)->code == PSN_APPLY)
        {
            setForceSync(SendMessage(GetDlgItem(hDlg, IDC_OLDSYNC), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);
            setSyncAudio(SendMessage(GetDlgItem(hDlg, IDC_AUDIOSYNC), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);
            setAIEmulation(SendMessage(GetDlgItem(hDlg, IDC_AI), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);
            setBufferLevel((unsigned long)SendMessage(GetDlgItem(hDlg, IDC_BUFFERS), TBM_GETPOS, 0, 0));
            setBackendFPS((unsigned long)SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BACKFPS), TBM_GETPOS, 0, 0) * 15);
            setBufferFPS((unsigned long)SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BUFFERFPS), TBM_GETPOS, 0, 0) * 15);
            setDisallowSleepDS8(
                SendMessage(GetDlgItem(hDlg, IDC_DISALLOWDS8), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);
            setDisallowSleepXA2(
                SendMessage(GetDlgItem(hDlg, IDC_DISALLOWXA2), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);
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
                dwPosition = (unsigned long)SendMessage(GetDlgItem(hDlg, IDC_BUFFERS), TBM_GETPOS, 0, 0);
                sprintf(textPos, "%li", dwPosition);
                SetDlgItemText(hDlg, IDC_BUFFERS_TEXT, (LPCSTR)textPos);
                break;
            case IDC_SLIDER_BACKFPS:
                dwPosition = (unsigned long)SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BACKFPS), TBM_GETPOS, 0, 0);
                sprintf(textPos, "%li ms", (DWORD)(1000 / (dwPosition * 15)));
                SetDlgItemText(hDlg, IDC_SLIDER_BACKFPS_TEXT, (LPCSTR)textPos);
                break;
            case IDC_SLIDER_BUFFERFPS:
                dwPosition = (unsigned long)SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BUFFERFPS), TBM_GETPOS, 0, 0);
                sprintf(textPos, "%li ms", (DWORD)(1000 / (dwPosition * 15)));
                SetDlgItemText(hDlg, IDC_SLIDER_BUFFERFPS_TEXT, (LPCSTR)textPos);
                break;
            }
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

INT_PTR CALLBACK Configuration::SettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int x;
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SendMessage(GetDlgItem(hDlg, IDC_DEVICE), CB_RESETCONTENT, 0, 0);
        SendMessage(GetDlgItem(hDlg, IDC_DEVICE), CB_ADDSTRING, 0, (LPARAM)(char *)"Default");
        SendMessage(GetDlgItem(hDlg, IDC_DEVICE), CB_SETCURSEL, 0, 0);
        SendMessage(GetDlgItem(hDlg, IDC_VOLUME), TBM_SETPOS, TRUE, configVolume);
        SendMessage(GetDlgItem(hDlg, IDC_VOLUME), TBM_SETTICFREQ, 20, 0);
        SendMessage(GetDlgItem(hDlg, IDC_VOLUME), TBM_SETRANGEMIN, FALSE, 0);
        SendMessage(GetDlgItem(hDlg, IDC_VOLUME), TBM_SETRANGEMAX, FALSE, 100);
        if (configVolume == 100)
        {
            SendMessage(GetDlgItem(hDlg, IDC_MUTE), BM_SETCHECK, BST_CHECKED, 0);
        }
        else
        {
            SendMessage(GetDlgItem(hDlg, IDC_MUTE), BM_SETCHECK, BST_UNCHECKED, 0);
        }
        break;
    case WM_NOTIFY:
        if (((NMHDR FAR *)lParam)->code == PSN_APPLY)
        {
            configVolume = (unsigned long)SendMessage(GetDlgItem(hDlg, IDC_VOLUME), TBM_GETPOS, 0, 0);
            snd->SetVolume(configVolume);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_MUTE: {
            if (IsDlgButtonChecked(hDlg, IDC_MUTE))
            {
                SendMessage(GetDlgItem(hDlg, IDC_VOLUME), TBM_SETPOS, TRUE, 100);
                snd->SetVolume(100);
            }
            else
            {
                SendMessage(GetDlgItem(hDlg, IDC_VOLUME), TBM_SETPOS, TRUE, configVolume);
                snd->SetVolume(configVolume);
            }
        }
        }
        break;
    case WM_VSCROLL: {
        short int userReq = LOWORD(wParam);
        if (userReq == TB_ENDTRACK || userReq == TB_THUMBTRACK)
        {
            unsigned long dwPosition = (unsigned long)SendMessage(GetDlgItem(hDlg, IDC_VOLUME), TBM_GETPOS, 0, 0);
            if (dwPosition == 100)
            {
                SendMessage(GetDlgItem(hDlg, IDC_MUTE), BM_SETCHECK, BST_CHECKED, 0);
            }
            else
            {
                SendMessage(GetDlgItem(hDlg, IDC_MUTE), BM_SETCHECK, BST_UNCHECKED, 0);
            }
            configVolume = dwPosition;
            snd->SetVolume(dwPosition);
        }
        break;
    }
    default:
        return FALSE;
    }
    return TRUE;
}
