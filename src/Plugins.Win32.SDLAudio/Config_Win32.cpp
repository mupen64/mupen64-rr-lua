#include "Config_Win32.hpp"
#include "Main_Win32.hpp"
#include "Resource.h"

#include <minwindef.h>
#include <windows.h>
#include <winuser.h>

static __stdcall int config_dlgproc(HWND dialog, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CLOSE: // "close" button clicked
        EndDialog(dialog, IDCANCEL);
        break;
    case WM_COMMAND: // bottom button clicked
        switch (LOWORD(wparam))
        {
        case IDOK:
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
void show_config_win32(HWND parent, Config &config)
{
    DialogBoxW(g_dll_handle, MAKEINTRESOURCE(IDD_CONFIG), parent, config_dlgproc);
}
} // namespace SDLAudio