/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Config_Win32.hpp"
#include "Main_Win32.hpp"
#include "Resource.h"

#include <format>
#include <commctrl.h>
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

// Macros Windows should really have, but don't.
#define Trackbar_GetPos(hwndCtl) ((int)(DWORD)SendMessage((hwndCtl), TBM_GETPOS, 0L, 0L))
#define Trackbar_SetRangeMin(hwndCtl, value) ((int)(DWORD)SendMessage((hwndCtl), TBM_SETRANGEMIN, 0L, (value)))
#define Trackbar_SetRangeMax(hwndCtl, value) ((int)(DWORD)SendMessage((hwndCtl), TBM_SETRANGEMAX, 0L, (value)))
#define Trackbar_SetTickFreq(hwndCtl, value) ((int)(DWORD)SendMessage((hwndCtl), TBM_SETTICFREQ, 1L, (value)))
#define Trackbar_SetPos(hwndCtl, value) ((int)(DWORD)SendMessage((hwndCtl), TBM_SETPOS, 1L, (value)))

static SDLAudio::Config *g_config_ptr = nullptr;

static CALLBACK INT_PTR config_dlgproc(HWND dialog, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        Button_SetCheck(GetDlgItem(dialog, IDC_SWAP_CHANNELS), g_config_ptr->swap_channels);
        Button_SetCheck(GetDlgItem(dialog, IDC_SYNC_AUDIO), g_config_ptr->sync_audio);

        {
            HWND idc_volume = GetDlgItem(dialog, IDC_VOLUME);
            Trackbar_SetRangeMin(idc_volume, 0);
            Trackbar_SetRangeMax(idc_volume, 100);
            Trackbar_SetTickFreq(idc_volume, 10);
            Trackbar_SetPos(idc_volume, (DWORD)g_config_ptr->volume_pct);
        }
        SetDlgItemText(dialog, IDC_VOLUME_TXT, std::format(L"{}%", (DWORD)g_config_ptr->volume_pct).c_str());

        break;
    case WM_CLOSE: // "close" button clicked
        EndDialog(dialog, IDCANCEL);
        break;
    case WM_COMMAND: // bottom button clicked
        switch (LOWORD(wparam))
        {
        case IDOK:
            g_config_ptr->swap_channels = !!Button_GetCheck(GetDlgItem(dialog, IDC_SWAP_CHANNELS));
            g_config_ptr->sync_audio = !!Button_GetCheck(GetDlgItem(dialog, IDC_SYNC_AUDIO));
            g_config_ptr->volume_pct = (uint8_t)Trackbar_GetPos(GetDlgItem(dialog, IDC_VOLUME));
            EndDialog(dialog, IDOK);
            break;
        case IDCANCEL:
            EndDialog(dialog, IDCANCEL);
            break;
        default:
            break;
        }
        break;
    case WM_HSCROLL:
        if (lparam == 0) break;

        switch (GetDlgCtrlID((HWND)lparam))
        {
        case IDC_VOLUME: {
            DWORD pos = Trackbar_GetPos(GetDlgItem(dialog, IDC_VOLUME));
            SetDlgItemText(dialog, IDC_VOLUME_TXT, std::format(L"{}%", pos).c_str());
            break;
        }
        default:
            break;
        }
    default:
        break;
    }
    return FALSE;
}

namespace SDLAudio
{
bool win32_show_config(HWND parent, Config &config)
{
    g_config_ptr = &config;
    LRESULT res = DialogBox(g_dll_handle, MAKEINTRESOURCE(IDD_CONFIG), parent, &config_dlgproc);
    g_config_ptr = nullptr;

    return res == IDOK;
}
} // namespace SDLAudio