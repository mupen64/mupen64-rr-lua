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
    std::variant<std::monostate, t_button_mapping *, t_axis_mapping *> target_value{};
};

static config_dialog_context g_ctx;

static void update_editbox(int id, const t_button_mapping &mapping)
{
    if (mapping.button != SDL_GAMEPAD_BUTTON_INVALID)
    {
        const auto str = IOUtils::to_wide_string(SDL_GetGamepadStringForButton((SDL_GamepadButton)mapping.button));
        SetDlgItemText(g_ctx.hwnd, id, str.c_str());
        return;
    }

    if (mapping.key != SDL_SCANCODE_UNKNOWN)
    {
        const auto str = IOUtils::to_wide_string(SDL_GetScancodeName((SDL_Scancode)mapping.key));
        SetDlgItemText(g_ctx.hwnd, id, str.c_str());
        return;
    }
}

static void update_editbox(int id_negative, int id_positive, const t_axis_mapping &mapping)
{
    if (mapping.axis != SDL_GAMEPAD_AXIS_INVALID)
    {
        const auto str = IOUtils::to_wide_string(SDL_GetGamepadStringForAxis((SDL_GamepadAxis)mapping.axis));
        SetDlgItemText(g_ctx.hwnd, id_negative, str.c_str());
        SetDlgItemText(g_ctx.hwnd, id_positive, str.c_str());
        return;
    }

    if (mapping.key_negative != SDL_SCANCODE_UNKNOWN)
    {
        const auto str = IOUtils::to_wide_string(SDL_GetScancodeName((SDL_Scancode)mapping.key_negative));
        SetDlgItemText(g_ctx.hwnd, id_negative, str.c_str());
    }

    if (mapping.key_positive != SDL_SCANCODE_UNKNOWN)
    {
        const auto str = IOUtils::to_wide_string(SDL_GetScancodeName((SDL_Scancode)mapping.key_positive));
        SetDlgItemText(g_ctx.hwnd, id_positive, str.c_str());
    }
}

static void update_editboxes()
{
    const auto buttons = {IDC_E_A,      IDC_E_B,       IDC_E_START, IDC_E_ZTRIG,   IDC_E_LTRIG, IDC_E_RTRIG,
                          IDC_E_DPLEFT, IDC_E_DPRIGHT, IDC_E_DPUP,  IDC_E_DPDOWN,  IDC_E_CLEFT, IDC_E_CRIGHT,
                          IDC_E_CUP,    IDC_E_CDOWN,   IDC_EAS_UP,  IDC_EAS_RIGHT, IDC_EAS_UP,  IDC_EAS_DOWN};

    for (const auto btn : buttons)
    {
        SetDlgItemText(g_ctx.hwnd, btn, L"");
    }

    update_editbox(IDC_E_A, new_config.controller_config.a);
    update_editbox(IDC_E_B, new_config.controller_config.b);
    update_editbox(IDC_E_START, new_config.controller_config.start);

    update_editbox(IDC_E_ZTRIG, new_config.controller_config.z);
    update_editbox(IDC_E_LTRIG, new_config.controller_config.l);
    update_editbox(IDC_E_RTRIG, new_config.controller_config.r);

    update_editbox(IDC_E_DPLEFT, new_config.controller_config.dpad_left);
    update_editbox(IDC_E_DPRIGHT, new_config.controller_config.dpad_right);
    update_editbox(IDC_E_DPUP, new_config.controller_config.dpad_up);
    update_editbox(IDC_E_DPDOWN, new_config.controller_config.dpad_down);

    update_editbox(IDC_E_CLEFT, new_config.controller_config.c_left);
    update_editbox(IDC_E_CRIGHT, new_config.controller_config.c_right);
    update_editbox(IDC_E_CUP, new_config.controller_config.c_up);
    update_editbox(IDC_E_CDOWN, new_config.controller_config.c_down);
}

static bool is_editing()
{
    return !std::holds_alternative<std::monostate>(g_ctx.target_value);
}

static void end_edit()
{
    g_ctx.target_value = std::monostate{};
    update_editboxes();
}

static void pre_begin_edit(int edit_id)
{
    if (is_editing())
    {
        end_edit();
    }

    SetDlgItemText(g_ctx.hwnd, edit_id, L"...");
}

static void begin_edit(int edit_id, t_button_mapping *ptr)
{
    pre_begin_edit(edit_id);

    g_ctx.target_value = ptr;
}

static void begin_edit(int edit_id, t_axis_mapping *ptr)
{
    pre_begin_edit(edit_id);

    g_ctx.target_value = ptr;
}

#define HANDLE_EDIT_BEGIN(btn_id, editbox_id, ptr)                                                                     \
    case btn_id:                                                                                                       \
        begin_edit(editbox_id, ptr);                                                                                   \
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

            HANDLE_EDIT_BEGIN(IDC_B_A, IDC_E_A, &new_config.controller_config.a)
            HANDLE_EDIT_BEGIN(IDC_B_B, IDC_E_B, &new_config.controller_config.b)
            HANDLE_EDIT_BEGIN(IDC_B_START, IDC_E_START, &new_config.controller_config.start)

            HANDLE_EDIT_BEGIN(IDC_B_ZTRIG, IDC_E_ZTRIG, &new_config.controller_config.z)
            HANDLE_EDIT_BEGIN(IDC_B_LTRIG, IDC_E_LTRIG, &new_config.controller_config.l)
            HANDLE_EDIT_BEGIN(IDC_B_RTRIG, IDC_E_RTRIG, &new_config.controller_config.r)

            HANDLE_EDIT_BEGIN(IDC_B_DPLEFT, IDC_E_DPLEFT, &new_config.controller_config.dpad_left)
            HANDLE_EDIT_BEGIN(IDC_B_DPRIGHT, IDC_E_DPRIGHT, &new_config.controller_config.dpad_right)
            HANDLE_EDIT_BEGIN(IDC_B_DPUP, IDC_E_DPUP, &new_config.controller_config.dpad_up)
            HANDLE_EDIT_BEGIN(IDC_B_DPDOWN, IDC_E_DPDOWN, &new_config.controller_config.dpad_down)

            HANDLE_EDIT_BEGIN(IDC_B_CLEFT, IDC_E_CLEFT, &new_config.controller_config.c_left)
            HANDLE_EDIT_BEGIN(IDC_B_CRIGHT, IDC_E_CRIGHT, &new_config.controller_config.c_right)
            HANDLE_EDIT_BEGIN(IDC_B_CUP, IDC_E_CUP, &new_config.controller_config.c_up)
            HANDLE_EDIT_BEGIN(IDC_B_CDOWN, IDC_E_CDOWN, &new_config.controller_config.c_down)

            HANDLE_EDIT_BEGIN(IDC_BAS_LEFT, IDC_EAS_LEFT, &new_config.controller_config.x)
            HANDLE_EDIT_BEGIN(IDC_BAS_RIGHT, IDC_EAS_RIGHT, &new_config.controller_config.x)
            HANDLE_EDIT_BEGIN(IDC_BAS_UP, IDC_EAS_UP, &new_config.controller_config.y)
            HANDLE_EDIT_BEGIN(IDC_BAS_DOWN, IDC_EAS_DOWN, &new_config.controller_config.y)

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
    if (!is_editing())
    {
        return;
    }

    if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
    {
        if (auto *mapping = std::get_if<t_button_mapping *>(&g_ctx.target_value))
        {
            (*mapping)->button = e.gbutton.button;
            (*mapping)->key = SDL_SCANCODE_UNKNOWN;
            end_edit();
        }
    }

    if (e.type == SDL_EVENT_GAMEPAD_AXIS_MOTION)
    {
        if (auto *mapping = std::get_if<t_axis_mapping *>(&g_ctx.target_value))
        {
            const int16_t axis_value = e.gaxis.value;
            const int32_t threshold = 16000;

            if (axis_value < -threshold)
            {
                (*mapping)->axis = e.gaxis.axis;
                (*mapping)->key_negative = SDL_SCANCODE_UNKNOWN;
                (*mapping)->key_positive = SDL_SCANCODE_UNKNOWN;
                end_edit();
            }

            else if (axis_value > threshold)
            {
                (*mapping)->axis = e.gaxis.axis;
                (*mapping)->key_negative = SDL_SCANCODE_UNKNOWN;
                (*mapping)->key_positive = SDL_SCANCODE_UNKNOWN;
                end_edit();
            }
        }
    }

    if (e.type == SDL_EVENT_KEY_DOWN)
    {
        if (auto *mapping = std::get_if<t_button_mapping *>(&g_ctx.target_value))
        {
            (*mapping)->button = SDL_GAMEPAD_BUTTON_INVALID;
            (*mapping)->key = e.key.scancode;
            end_edit();
        }

        if (auto *mapping = std::get_if<t_axis_mapping *>(&g_ctx.target_value))
        {
            (*mapping)->axis = SDL_GAMEPAD_AXIS_INVALID;
            (*mapping)->key_negative = SDL_SCANCODE_UNKNOWN;
            (*mapping)->key_positive = SDL_SCANCODE_UNKNOWN;
            end_edit();
        }
    }
}
