/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */


#include "stdafx.h"
#include <ConfigDialog.h>
#include <Main.h>

static LRESULT CALLBACK dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDOK:
            EndDialog(hwnd, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        default:
            break;
        }
    default:
        break;
    }
    return FALSE;
}
void cfgdiag_show(HWND parent)
{
    DialogBox(g_inst, MAKEINTRESOURCE(IDD_CONFIGDLG), parent, (DLGPROC)dlgproc);
}
