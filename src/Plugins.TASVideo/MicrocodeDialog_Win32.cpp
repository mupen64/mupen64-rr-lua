/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "MicrocodeDialog_Win32.hpp"
#include "TASVideo.hpp"
#include "Resource.h"
#include "GBI.hpp"
#include <Common.Win32/Common.hpp>

INT_PTR CALLBACK MicrocodeDlgProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG: {
        EnableMenuItem(GetSystemMenu(hWndDlg, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

        for (int i = 0; i < numMicrocodeTypes; i++)
        {
            ComboBox_AddString(GetDlgItem(hWndDlg, IDC_MICROCODE), MicrocodeTypes[i]);
        }
        SendDlgItemMessage(hWndDlg, IDC_MICROCODE, CB_SETCURSEL, 0, 0);

        char text[1024]{};
        sprintf(text, "Microcode CRC:\t\t0x%08x\r\nMicrocode Data CRC:\t0x%08x\r\nMicrocode Text:\t\t%s", uc_crc,
            uc_dcrc, uc_str);
        Edit_SetText(GetDlgItem(hWndDlg, IDC_TEXTBOX), text);
        return TRUE;
    }
    case WM_CLOSE:
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            EndDialog(hWndDlg, SendDlgItemMessage(hWndDlg, IDC_MICROCODE, CB_GETCURSEL, 0, 0));
            return TRUE;
        }
        break;
    }

    return FALSE;
}

uint32_t MicrocodeDialog::show()
{
    return DialogBox(
        GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_MICROCODEDLG), g_plugin->main_window.hwnd(), MicrocodeDlgProc);
}
