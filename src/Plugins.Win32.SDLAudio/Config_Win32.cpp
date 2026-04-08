#include "Config_Win32.hpp"
#include "Main_Win32.hpp"
#include "Resource.h"

#include <minwindef.h>
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

static SDLAudio::Config *g_config_ptr = nullptr;

static CALLBACK INT_PTR config_dlgproc(HWND dialog, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        Button_SetCheck(GetDlgItem(dialog, IDC_SWAP_CHANNELS), g_config_ptr->swap_channels);
        Button_SetCheck(GetDlgItem(dialog, IDC_SYNC_AUDIO), g_config_ptr->sync_audio);
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
            EndDialog(dialog, IDOK);
            break;
        case IDCANCEL:
            EndDialog(dialog, IDCANCEL);
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

namespace SDLAudio
{
bool show_config_win32(HWND parent, Config &config)
{
    g_config_ptr = &config;
    LRESULT res = DialogBoxW(g_dll_handle, MAKEINTRESOURCE(IDD_CONFIG), parent, &config_dlgproc);
    g_config_ptr = nullptr;

    return res == IDOK;
}
} // namespace SDLAudio