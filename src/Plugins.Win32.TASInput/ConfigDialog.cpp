/*
 * Copyright (c) 2025, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.h"
#include <ConfigDialog.h>
#include <Main.h>
#include <NewConfig.h>
#include <GamepadManager.h>

static void update_axis_editbox(HWND hwnd, int id, int value)
{
    if (value == SDL_GAMEPAD_AXIS_INVALID)
    {
        SetDlgItemText(hwnd, id, L"");
        return;
    }

    const auto str = SDL_GetGamepadStringForAxis((SDL_GamepadAxis)value);
    SetDlgItemText(hwnd, id, IOUtils::to_wide_string(str).c_str());
}

static void update_button_editbox(HWND hwnd, int id, int value)
{
    if (value == SDL_GAMEPAD_BUTTON_INVALID)
    {
        SetDlgItemText(hwnd, id, L"");
        return;
    }

    const auto str = SDL_GetGamepadStringForButton((SDL_GamepadButton)value);
    SetDlgItemText(hwnd, id, IOUtils::to_wide_string(str).c_str());
}

static LRESULT CALLBACK dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG: {
        update_button_editbox(hwnd, IDC_E_A, new_config.controller_config.a);
        update_button_editbox(hwnd, IDC_E_B, new_config.controller_config.b);
        update_button_editbox(hwnd, IDC_E_START, new_config.controller_config.start);
        update_button_editbox(hwnd, IDC_E_ZTRIG, new_config.controller_config.z);
        update_button_editbox(hwnd, IDC_E_LTRIG, new_config.controller_config.l);
        update_button_editbox(hwnd, IDC_E_RTRIG, new_config.controller_config.r);

        update_button_editbox(hwnd, IDC_E_DPLEFT, new_config.controller_config.dpad_left);
        update_button_editbox(hwnd, IDC_E_DPRIGHT, new_config.controller_config.dpad_right);
        update_button_editbox(hwnd, IDC_E_DPUP, new_config.controller_config.dpad_up);
        update_button_editbox(hwnd, IDC_E_DPDOWN, new_config.controller_config.dpad_down);

        update_button_editbox(hwnd, IDC_E_CLEFT, new_config.controller_config.c_left);
        update_button_editbox(hwnd, IDC_E_CRIGHT, new_config.controller_config.c_right);
        update_button_editbox(hwnd, IDC_E_CUP, new_config.controller_config.c_up);
        update_button_editbox(hwnd, IDC_E_CDOWN, new_config.controller_config.c_down);

        update_axis_editbox(hwnd, IDC_EAS_LEFT, new_config.controller_config.x);
        update_axis_editbox(hwnd, IDC_EAS_RIGHT, new_config.controller_config.x);
        update_axis_editbox(hwnd, IDC_EAS_UP, new_config.controller_config.y);
        update_axis_editbox(hwnd, IDC_EAS_DOWN, new_config.controller_config.y);
        break;
    }
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
        case IDC_B_A:

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
