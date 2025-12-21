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

struct config_dialog_context
{
    HWND hwnd{};
    int32_t *editing_button{};
};

static config_dialog_context g_ctx;

static void update_axis_editbox(int id, int value)
{
    if (value == SDL_GAMEPAD_AXIS_INVALID)
    {
        SetDlgItemText(g_ctx.hwnd, id, L"");
        return;
    }

    const auto str = SDL_GetGamepadStringForAxis((SDL_GamepadAxis)value);
    SetDlgItemText(g_ctx.hwnd, id, IOUtils::to_wide_string(str).c_str());
}

static void update_button_editbox(int id, int value)
{
    if (value == SDL_GAMEPAD_BUTTON_INVALID)
    {
        SetDlgItemText(g_ctx.hwnd, id, L"");
        return;
    }

    const auto str = SDL_GetGamepadStringForButton((SDL_GamepadButton)value);
    SetDlgItemText(g_ctx.hwnd, id, IOUtils::to_wide_string(str).c_str());
}

static void update_editboxes()
{
    update_button_editbox(IDC_E_A, new_config.controller_config.a);
    update_button_editbox(IDC_E_B, new_config.controller_config.b);
    update_button_editbox(IDC_E_START, new_config.controller_config.start);
    update_button_editbox(IDC_E_ZTRIG, new_config.controller_config.z);
    update_button_editbox(IDC_E_LTRIG, new_config.controller_config.l);
    update_button_editbox(IDC_E_RTRIG, new_config.controller_config.r);

    update_button_editbox(IDC_E_DPLEFT, new_config.controller_config.dpad_left);
    update_button_editbox(IDC_E_DPRIGHT, new_config.controller_config.dpad_right);
    update_button_editbox(IDC_E_DPUP, new_config.controller_config.dpad_up);
    update_button_editbox(IDC_E_DPDOWN, new_config.controller_config.dpad_down);

    update_button_editbox(IDC_E_CLEFT, new_config.controller_config.c_left);
    update_button_editbox(IDC_E_CRIGHT, new_config.controller_config.c_right);
    update_button_editbox(IDC_E_CUP, new_config.controller_config.c_up);
    update_button_editbox(IDC_E_CDOWN, new_config.controller_config.c_down);

    update_axis_editbox(IDC_EAS_LEFT, new_config.controller_config.x);
    update_axis_editbox(IDC_EAS_RIGHT, new_config.controller_config.x);
    update_axis_editbox(IDC_EAS_UP, new_config.controller_config.y);
    update_axis_editbox(IDC_EAS_DOWN, new_config.controller_config.y);
}

static void begin_button_edit(int btn_id, int edit_id, int32_t *ptr)
{
    if (g_ctx.editing_button)
    {
        g_ctx.editing_button = nullptr;
        update_editboxes();
    }

    SetDlgItemText(g_ctx.hwnd, edit_id, L"...");

    g_ctx.editing_button = ptr;
}

#define HANDLE_BUTTON(btn_id, editbox_id, ptr)                                                                         \
    case btn_id:                                                                                                       \
        begin_button_edit(btn_id, editbox_id, ptr);                                                                    \
        break;

static LRESULT CALLBACK dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        g_ctx.hwnd = hwnd;
        update_editboxes();
        break;
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

            HANDLE_BUTTON(IDC_B_A, IDC_E_A, &new_config.controller_config.a)
            HANDLE_BUTTON(IDC_B_B, IDC_E_B, &new_config.controller_config.b)
            HANDLE_BUTTON(IDC_B_START, IDC_E_START, &new_config.controller_config.start)

            HANDLE_BUTTON(IDC_B_ZTRIG, IDC_E_ZTRIG, &new_config.controller_config.z)
            HANDLE_BUTTON(IDC_B_LTRIG, IDC_E_LTRIG, &new_config.controller_config.l)
            HANDLE_BUTTON(IDC_B_RTRIG, IDC_E_RTRIG, &new_config.controller_config.r)

            HANDLE_BUTTON(IDC_B_DPLEFT, IDC_E_DPLEFT, &new_config.controller_config.dpad_left)
            HANDLE_BUTTON(IDC_B_DPRIGHT, IDC_E_DPRIGHT, &new_config.controller_config.dpad_right)
            HANDLE_BUTTON(IDC_B_DPUP, IDC_E_DPUP, &new_config.controller_config.dpad_up)
            HANDLE_BUTTON(IDC_B_DPDOWN, IDC_E_DPDOWN, &new_config.controller_config.dpad_down)

            HANDLE_BUTTON(IDC_B_CLEFT, IDC_E_CLEFT, &new_config.controller_config.c_left)
            HANDLE_BUTTON(IDC_B_CRIGHT, IDC_E_CRIGHT, &new_config.controller_config.c_right)
            HANDLE_BUTTON(IDC_B_CUP, IDC_E_CUP, &new_config.controller_config.c_up)
            HANDLE_BUTTON(IDC_B_CDOWN, IDC_E_CDOWN, &new_config.controller_config.c_down)

        default:
            break;
        }
    default:
        break;
    }
    return FALSE;
}

void ConfigDialog::show(HWND parent)
{
    DialogBox(g_inst, MAKEINTRESOURCE(IDD_CONFIGDLG), parent, (DLGPROC)dlgproc);
}

void ConfigDialog::on_sdl_event(const SDL_Event &e)
{
    if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
    {
        if (g_ctx.editing_button)
        {
            *g_ctx.editing_button = e.gbutton.button;
            g_ctx.editing_button = nullptr;
            update_editboxes();
        }
    }
}
