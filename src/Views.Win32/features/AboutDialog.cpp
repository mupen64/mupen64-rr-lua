#include "stdafx.h"
#include "AboutDialog.h"

static INT_PTR CALLBACK about_dlg_proc(const HWND hwnd, const UINT message, const WPARAM w_param, LPARAM)
{
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hwnd, IDC_VERSION_TEXT, get_mupen_name().c_str());
        break;
    case WM_CLOSE:
        EndDialog(hwnd, IDOK);
        break;
    case WM_COMMAND:
        switch (LOWORD(w_param))
        {
        case IDOK:
            EndDialog(hwnd, IDOK);
            break;
        case IDC_WEBSITE:
            ShellExecute(nullptr, nullptr, L"http://mupen64.emulation64.com", nullptr, nullptr, SW_SHOW);
            break;
        case IDC_GITREPO:
            ShellExecute(nullptr, nullptr, L"https://github.com/mkdasher/mupen64-rr-lua-/", nullptr, nullptr, SW_SHOW);
            break;
        default:
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

void AboutDialog::show()
{
    DialogBox(g_app_instance, MAKEINTRESOURCE(IDD_ABOUT), g_main_hwnd, about_dlg_proc);
}
