/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Configuration.h"
#include "resource.h"
#include "SoundDriverInterface.h"
#include "SoundDriverFactory.h"
#include <windows.h>
#include <cassert>
#include <commctrl.h>

#define CONFIGFILENAME "AzimersLegacyAudio.ini"

#define KEY_INTNAME "INTERNAL_NAME"
#define KEY_SYNCAUDIO "SyncAudio"
#define KEY_FORCESYNC "ForceSync"
#define KEY_AIEMULATION "AIEmulation"
#define KEY_VOLUME "Volume"
#define KEY_DRIVER "Driver"
#define KEY_BUFFERLEVEL "BufferLevel"
#define KEY_BUFFERFPS "BufferFPS"
#define KEY_BACKENDFPS "BackendFPS"
#define KEY_DISALLOWSLEEPXA2 "DisallowSleepXA2"
#define KEY_DISALLOWSLEEPDS8 "DisallowSleepDS8"

#define SECTION_GENERAL "DEFAULT_SETTINGS"

extern HINSTANCE hInstance; // DLL's HINSTANCE

extern SoundDriverInterface* snd;

t_romheader* Configuration::Header;
bool Configuration::RomRunning = false;
unsigned long Configuration::configVolume;
char Configuration::configAudioLogFolder[MAX_FOLDER_LENGTH];
Settings Configuration::currentSettings;
SoundDriverType Configuration::configDriver;
static SoundDriverType EnumDriverType[10];
static int EnumDriverCount;

INT_PTR CALLBACK ConfigProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AdvancedProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool Configuration::config_load()
{
    // setResTimer(false);

    // CSimpleIniA ini;

    // SI_Error rc = ini.LoadFile(CONFIGFILENAME);
    // if (rc < 0)
    // {
    //     LoadDefaults();
    //     config_save();
    //     return false;
    // }

    // setSyncAudio(ini.GetBoolValue(SECTION_GENERAL, KEY_SYNCAUDIO, false));
    // setForceSync(ini.GetBoolValue(SECTION_GENERAL, KEY_FORCESYNC, false));
    // setAIEmulation(ini.GetBoolValue(SECTION_GENERAL, KEY_AIEMULATION, false));
    // setVolume(ini.GetLongValue(SECTION_GENERAL, KEY_VOLUME, 0));
    // setDriver((SoundDriverType)ini.GetLongValue(SECTION_GENERAL, KEY_DRIVER, (long)SoundDriverFactory::DefaultDriver()));
    // setBufferLevel(ini.GetLongValue(SECTION_GENERAL, KEY_BUFFERLEVEL, 3));
    // setBufferFPS(ini.GetLongValue(SECTION_GENERAL, KEY_BUFFERFPS, 45));
    // setBackendFPS(ini.GetLongValue(SECTION_GENERAL, KEY_BACKENDFPS, 90));
    // setDisallowSleepXA2(ini.GetBoolValue(SECTION_GENERAL, KEY_DISALLOWSLEEPXA2, false));
    // setDisallowSleepDS8(ini.GetBoolValue(SECTION_GENERAL, KEY_DISALLOWSLEEPDS8, false));
    return true;
}

bool Configuration::config_load_rom()
{
    // char CRC_Entry[128];

    // sprintf(CRC_Entry, "%08X-%08X-C:%02X", Header->crc1, Header->crc2, Header->countrycode);

    // CSimpleIniA ini;

    // SI_Error rc = ini.LoadFile(CONFIGFILENAME);
    // if (rc < 0)
    // {
    //     return false;
    // }

    // setSyncAudio(ini.GetBoolValue(CRC_Entry, KEY_SYNCAUDIO, getSyncAudio()));
    // setForceSync(ini.GetBoolValue(CRC_Entry, KEY_FORCESYNC, getForceSync()));
    // setAIEmulation(ini.GetBoolValue(CRC_Entry, KEY_AIEMULATION, getAIEmulation()));
    // setVolume(ini.GetLongValue(SECTION_GENERAL, KEY_VOLUME, getVolume()));
    // setDriver((SoundDriverType)ini.GetLongValue(SECTION_GENERAL, KEY_DRIVER, getDriver()));
    // setBufferLevel(ini.GetLongValue(CRC_Entry, KEY_BUFFERLEVEL, getBufferLevel()));
    // setBufferFPS(ini.GetLongValue(CRC_Entry, KEY_BUFFERFPS, getBufferFPS()));
    // setBackendFPS(ini.GetLongValue(CRC_Entry, KEY_BACKENDFPS, getBackendFPS()));
    // setDisallowSleepXA2(ini.GetBoolValue(SECTION_GENERAL, KEY_DISALLOWSLEEPXA2, getDisallowSleepXA2()));
    // setDisallowSleepDS8(ini.GetBoolValue(SECTION_GENERAL, KEY_DISALLOWSLEEPDS8, getDisallowSleepDS8()));
    return true;
}

void Configuration::LoadSettings()
{
    config_load();

    if (RomRunning)
        config_load_rom();

    if (snd && RomRunning)
        snd->SetVolume(configVolume);
}

bool Configuration::config_save()
{
    // CSimpleIniA ini;

    // SI_Error rc = ini.LoadFile(CONFIGFILENAME);
    // if (rc < 0)
    // {
    //     ini.Reset();
    // }

    // ini.SetLongValue(SECTION_GENERAL, KEY_SYNCAUDIO, getSyncAudio());
    // ini.SetLongValue(SECTION_GENERAL, KEY_FORCESYNC, getForceSync());
    // ini.SetLongValue(SECTION_GENERAL, KEY_AIEMULATION, getAIEmulation());
    // ini.SetLongValue(SECTION_GENERAL, KEY_VOLUME, getVolume());
    // ini.SetLongValue(SECTION_GENERAL, KEY_DRIVER, getDriver());
    // ini.SetLongValue(SECTION_GENERAL, KEY_BUFFERLEVEL, getBufferLevel());
    // ini.SetLongValue(SECTION_GENERAL, KEY_BUFFERFPS, getBufferFPS());
    // ini.SetLongValue(SECTION_GENERAL, KEY_BACKENDFPS, getBackendFPS());
    // ini.SetLongValue(SECTION_GENERAL, KEY_DISALLOWSLEEPXA2, getDisallowSleepXA2());
    // ini.SetLongValue(SECTION_GENERAL, KEY_DISALLOWSLEEPDS8, getDisallowSleepDS8());

    // rc = ini.SaveFile(CONFIGFILENAME);
    // if (rc < 0)
    //     return FALSE;

    // assert(rc == SI_OK);

    return TRUE;
}

bool Configuration::config_save_rom()
{
    // char temp[40];
    // char CRC_Entry[128];

    // sprintf(CRC_Entry, "%08X-%08X-C:%02X", Header->crc1, Header->crc2, Header->countrycode);
    // for (int i = 0; i < 32; i++)
    //     temp[i] = Header->name[i ^ 3];

    // temp[32] = 0;

    // CSimpleIniA ini;

    // SI_Error rc = ini.LoadFile(CONFIGFILENAME);
    // if (rc < 0)
    // {
    //     ini.Reset();
    // }

    // ini.SetValue(CRC_Entry, KEY_INTNAME, temp);
    // ini.SetLongValue(CRC_Entry, KEY_SYNCAUDIO, getSyncAudio());
    // ini.SetLongValue(CRC_Entry, KEY_FORCESYNC, getForceSync());
    // ini.SetLongValue(CRC_Entry, KEY_AIEMULATION, getAIEmulation());
    // ini.SetLongValue(SECTION_GENERAL, KEY_VOLUME, getVolume());
    // ini.SetLongValue(SECTION_GENERAL, KEY_DRIVER, getDriver());
    // ini.SetLongValue(CRC_Entry, KEY_BUFFERLEVEL, getBufferLevel());
    // ini.SetLongValue(CRC_Entry, KEY_BUFFERFPS, getBufferFPS());
    // ini.SetLongValue(CRC_Entry, KEY_BACKENDFPS, getBackendFPS());
    // ini.SetLongValue(SECTION_GENERAL, KEY_DISALLOWSLEEPXA2, getDisallowSleepXA2());
    // ini.SetLongValue(SECTION_GENERAL, KEY_DISALLOWSLEEPDS8, getDisallowSleepDS8());

    // rc = ini.SaveFile(CONFIGFILENAME);
    // if (rc < 0)
    //     return FALSE;

    // assert(rc == SI_OK);

    return TRUE;
}

void Configuration::SaveSettings()
{
    if (RomRunning)
    {
        config_save_rom();
        return;
    }

    config_save();
}

/*
    Loads the default values expected for Configuration.  This will also load the settings from a file if the configuration file exists
*/
void Configuration::LoadDefaults()
{
    strcpy_s(configAudioLogFolder, 499, "D:\\");
    strcpy(configAudioLogFolder, "D:\\");
    configVolume = 0;
    EnumDriverCount = SoundDriverFactory::EnumDrivers(EnumDriverType, 10); // TODO: This needs to be fixed.  10 is an arbitrary number which doesn't meet the 20 set in MAX_FACTORY_DRIVERS
    setSyncAudio(false);
    setForceSync(false);
    setAIEmulation(true);
    setVolume(0);
    setDriver(SoundDriverFactory::DefaultDriver());
    setBufferLevel(3);
    setBufferFPS(45);
    setBackendFPS(90);
    setDisallowSleepDS8(false);
    setDisallowSleepXA2(false);
    setFrequency(44100);
    setBitRate(16);
    setResTimer(false);
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
    Settings tmp = currentSettings;
    SendMessage(GetDlgItem(hDlg, IDC_OLDSYNC), BM_SETCHECK, tmp.configForceSync ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hDlg, IDC_AUDIOSYNC), BM_SETCHECK, tmp.configSyncAudio ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hDlg, IDC_AI), BM_SETCHECK, tmp.configAIEmulation ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hDlg, IDC_BUFFERS), TBM_SETTICFREQ, 1, 0);
    SendMessage(GetDlgItem(hDlg, IDC_BUFFERS), TBM_SETRANGEMIN, FALSE, 2);
    SendMessage(GetDlgItem(hDlg, IDC_BUFFERS), TBM_SETRANGEMAX, FALSE, 9);
    SendMessage(GetDlgItem(hDlg, IDC_BUFFERS), TBM_SETPOS, TRUE, tmp.configBufferLevel);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BACKFPS), TBM_SETTICFREQ, 1, 0);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BACKFPS), TBM_SETRANGEMIN, FALSE, 1);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BACKFPS), TBM_SETRANGEMAX, FALSE, 8);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BACKFPS), TBM_SETPOS, TRUE, tmp.configBackendFPS / 15);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BUFFERFPS), TBM_SETTICFREQ, 1, 0);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BUFFERFPS), TBM_SETRANGEMIN, FALSE, 1);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BUFFERFPS), TBM_SETRANGEMAX, FALSE, 8);
    SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BUFFERFPS), TBM_SETPOS, TRUE, tmp.configBufferFPS / 15);
    SendMessage(GetDlgItem(hDlg, IDC_DISALLOWDS8), BM_SETCHECK, tmp.configDisallowSleepDS8 ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hDlg, IDC_DISALLOWXA2), BM_SETCHECK, tmp.configDisallowSleepXA2 ? BST_CHECKED : BST_UNCHECKED, 0);
    char textPos[20];
    sprintf(textPos, "%li", tmp.configBufferLevel);
    SetDlgItemText(hDlg, IDC_BUFFERS_TEXT, (LPCSTR)textPos);
    sprintf(textPos, "%li ms", 1000 / tmp.configBackendFPS);
    SetDlgItemText(hDlg, IDC_SLIDER_BACKFPS_TEXT, (LPCSTR)textPos);
    sprintf(textPos, "%li ms", 1000 / tmp.configBufferFPS);
    SetDlgItemText(hDlg, IDC_SLIDER_BUFFERFPS_TEXT, (LPCSTR)textPos);
}

INT_PTR CALLBACK Configuration::AdvancedProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SendMessage(GetDlgItem(hDlg, IDC_PROFILE), CB_RESETCONTENT, 0, 0);
            ResetAdvancedPage(hDlg);
        }
        break;
    case WM_NOTIFY:
        if (((NMHDR FAR*)lParam)->code == PSN_APPLY)
        {
            setForceSync(SendMessage(GetDlgItem(hDlg, IDC_OLDSYNC), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);
            setSyncAudio(SendMessage(GetDlgItem(hDlg, IDC_AUDIOSYNC), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);
            setAIEmulation(SendMessage(GetDlgItem(hDlg, IDC_AI), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);
            setBufferLevel((unsigned long)SendMessage(GetDlgItem(hDlg, IDC_BUFFERS), TBM_GETPOS, 0, 0));
            setBackendFPS((unsigned long)SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BACKFPS), TBM_GETPOS, 0, 0) * 15);
            setBufferFPS((unsigned long)SendMessage(GetDlgItem(hDlg, IDC_SLIDER_BUFFERFPS), TBM_GETPOS, 0, 0) * 15);
            setDisallowSleepDS8(SendMessage(GetDlgItem(hDlg, IDC_DISALLOWDS8), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);
            setDisallowSleepXA2(SendMessage(GetDlgItem(hDlg, IDC_DISALLOWXA2), BM_GETSTATE, 0, 0) == BST_CHECKED ? true : false);
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
        SendMessage(GetDlgItem(hDlg, IDC_BACKEND), CB_RESETCONTENT, 0, 0);
        SendMessage(GetDlgItem(hDlg, IDC_DEVICE), CB_ADDSTRING, 0, (LPARAM)(char*)"Default");
        SendMessage(GetDlgItem(hDlg, IDC_DEVICE), CB_SETCURSEL, 0, 0);
        for (x = 0; x < EnumDriverCount; x++)
        {
            SendMessage(GetDlgItem(hDlg, IDC_BACKEND), CB_ADDSTRING, 0, (LPARAM)SoundDriverFactory::GetDriverDescription(EnumDriverType[x]));
            if (EnumDriverType[x] == configDriver)
                SendMessage(GetDlgItem(hDlg, IDC_BACKEND), CB_SETCURSEL, x, 0);
        }
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
        if (((NMHDR FAR*)lParam)->code == PSN_APPLY)
        {
            configVolume = (unsigned long)SendMessage(GetDlgItem(hDlg, IDC_VOLUME), TBM_GETPOS, 0, 0);
            snd->SetVolume(configVolume);

            x = (unsigned long)SendMessage(GetDlgItem(hDlg, IDC_BACKEND), CB_GETCURSEL, 0, 0);
            if (EnumDriverType[x] != configDriver)
                configDriver = EnumDriverType[x];
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_MUTE:
            {
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
    case WM_VSCROLL:
        {
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
