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

const auto editbox_ids = {IDC_E_A,      IDC_E_B,       IDC_E_START, IDC_E_ZTRIG,   IDC_E_LTRIG, IDC_E_RTRIG,
                          IDC_E_DPLEFT, IDC_E_DPRIGHT, IDC_E_DPUP,  IDC_E_DPDOWN,  IDC_E_CLEFT, IDC_E_CRIGHT,
                          IDC_E_CUP,    IDC_E_CDOWN,   IDC_EAS_UP,  IDC_EAS_RIGHT, IDC_EAS_UP,  IDC_EAS_DOWN};

const auto button_ids = {IDC_B_A,      IDC_B_B,       IDC_B_START, IDC_B_ZTRIG,   IDC_B_LTRIG,  IDC_B_RTRIG,
                         IDC_B_DPLEFT, IDC_B_DPRIGHT, IDC_B_DPUP,  IDC_B_DPDOWN,  IDC_B_CLEFT,  IDC_B_CRIGHT,
                         IDC_B_CUP,    IDC_B_CDOWN,   IDC_BAS_UP,  IDC_BAS_RIGHT, IDC_BAS_LEFT, IDC_BAS_DOWN};

struct config_dialog_context
{
    HWND hwnd{};
    t_config prev_config{};
    std::variant<std::monostate, t_button_mapping *, t_axis_mapping *> target_value{};
    bool positive_target_axis{};
};

static config_dialog_context g_ctx;

static std::wstring virtual_keycode_to_string(int k)
{
    wchar_t buf2[64]{};
    if ((k >= 0x30 && k <= 0x39) || (k >= 0x41 && k <= 0x5A))
        wsprintf(buf2, L"%c", static_cast<char>(k));
    else if (k >= VK_F1 && k <= VK_F24)
        wsprintf(buf2, L"F%d", k - (VK_F1 - 1));
    else if (k >= VK_NUMPAD0 && k <= VK_NUMPAD9)
        wsprintf(buf2, L"Num%d", k - VK_NUMPAD0);
    else
        switch (k)
        {
        case VK_LBUTTON:
            wcscpy(buf2, L"LMB");
            break;
        case VK_RBUTTON:
            wcscpy(buf2, L"RMB");
            break;
        case VK_MBUTTON:
            wcscpy(buf2, L"MMB");
            break;
        case VK_XBUTTON1:
            wcscpy(buf2, L"XMB1");
            break;
        case VK_XBUTTON2:
            wcscpy(buf2, L"XMB2");
            break;
        case VK_SPACE:
            wcscpy(buf2, L"Space");
            break;
        case VK_BACK:
            wcscpy(buf2, L"Backspace");
            break;
        case VK_TAB:
            wcscpy(buf2, L"Tab");
            break;
        case VK_CLEAR:
            wcscpy(buf2, L"Clear");
            break;
        case VK_RETURN:
            wcscpy(buf2, L"Enter");
            break;
        case VK_PAUSE:
            wcscpy(buf2, L"Pause");
            break;
        case VK_CAPITAL:
            wcscpy(buf2, L"Caps");
            break;
        case VK_PRIOR:
            wcscpy(buf2, L"PageUp");
            break;
        case VK_NEXT:
            wcscpy(buf2, L"PageDn");
            break;
        case VK_END:
            wcscpy(buf2, L"End");
            break;
        case VK_HOME:
            wcscpy(buf2, L"Home");
            break;
        case VK_LEFT:
            wcscpy(buf2, L"Left");
            break;
        case VK_UP:
            wcscpy(buf2, L"Up");
            break;
        case VK_RIGHT:
            wcscpy(buf2, L"Right");
            break;
        case VK_DOWN:
            wcscpy(buf2, L"Down");
            break;
        case VK_SELECT:
            wcscpy(buf2, L"Select");
            break;
        case VK_PRINT:
            wcscpy(buf2, L"Print");
            break;
        case VK_SNAPSHOT:
            wcscpy(buf2, L"PrintScrn");
            break;
        case VK_INSERT:
            wcscpy(buf2, L"Insert");
            break;
        case VK_DELETE:
            wcscpy(buf2, L"Delete");
            break;
        case VK_HELP:
            wcscpy(buf2, L"Help");
            break;
        case VK_MULTIPLY:
            wcscpy(buf2, L"Num*");
            break;
        case VK_ADD:
            wcscpy(buf2, L"Num+");
            break;
        case VK_SUBTRACT:
            wcscpy(buf2, L"Num-");
            break;
        case VK_DECIMAL:
            wcscpy(buf2, L"Num.");
            break;
        case VK_DIVIDE:
            wcscpy(buf2, L"Num/");
            break;
        case VK_NUMLOCK:
            wcscpy(buf2, L"NumLock");
            break;
        case VK_SCROLL:
            wcscpy(buf2, L"ScrollLock");
            break;
        case /*VK_OEM_PLUS*/ 0xBB:
            wcscpy(buf2, L"=+");
            break;
        case /*VK_OEM_MINUS*/ 0xBD:
            wcscpy(buf2, L"-_");
            break;
        case /*VK_OEM_COMMA*/ 0xBC:
            wcscpy(buf2, L",");
            break;
        case /*VK_OEM_PERIOD*/ 0xBE:
            wcscpy(buf2, L".");
            break;
        case VK_OEM_7:
            wcscpy(buf2, L"'\"");
            break;
        case VK_OEM_6:
            wcscpy(buf2, L"]}");
            break;
        case VK_OEM_5:
            wcscpy(buf2, L"\\|");
            break;
        case VK_OEM_4:
            wcscpy(buf2, L"[{");
            break;
        case VK_OEM_3:
            wcscpy(buf2, L"`~");
            break;
        case VK_OEM_2:
            wcscpy(buf2, L"/?");
            break;
        case VK_OEM_1:
            wcscpy(buf2, L";:");
            break;
        default:
            wsprintf(buf2, L"(%d)", k);
            break;
        }
    return buf2;
}

static void update_editbox(int id, const t_button_mapping &mapping)
{
    if (mapping.button != SDL_GAMEPAD_BUTTON_INVALID)
    {
        const auto str = IOUtils::to_wide_string(SDL_GetGamepadStringForButton((SDL_GamepadButton)mapping.button));
        SetDlgItemText(g_ctx.hwnd, id, str.c_str());
        return;
    }

    if (mapping.key != 0)
    {
        SetDlgItemText(g_ctx.hwnd, id, virtual_keycode_to_string(mapping.key).c_str());
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

    if (mapping.key_negative != 0)
    {
        SetDlgItemText(g_ctx.hwnd, id_negative, virtual_keycode_to_string(mapping.key_negative).c_str());
    }

    if (mapping.key_positive != 0)
    {
        SetDlgItemText(g_ctx.hwnd, id_positive, virtual_keycode_to_string(mapping.key_positive).c_str());
    }
}

static void update_editboxes()
{
    for (const auto btn : editbox_ids)
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

    update_editbox(IDC_EAS_LEFT, IDC_EAS_RIGHT, new_config.controller_config.x);
    update_editbox(IDC_EAS_UP, IDC_EAS_DOWN, new_config.controller_config.y);
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
        SetFocus(GetDlgItem(g_ctx.hwnd, btn_id));                                                                      \
        break;

static LRESULT CALLBACK hotkey_button_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id,
                                                    DWORD_PTR ref_data)
{
    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, hotkey_button_subclass_proc, id);
        break;
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS;
    case WM_CHAR:
        return TRUE;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:

        if (auto *mapping = std::get_if<t_button_mapping *>(&g_ctx.target_value))
        {
            (*mapping)->button = SDL_GAMEPAD_BUTTON_INVALID;
            (*mapping)->key = wparam;
            end_edit();
        }

        if (auto *mapping = std::get_if<t_axis_mapping *>(&g_ctx.target_value))
        {
            (*mapping)->axis = SDL_GAMEPAD_AXIS_INVALID;
            if (g_ctx.positive_target_axis)
                (*mapping)->key_positive = wparam;
            else
                (*mapping)->key_negative = wparam;
            end_edit();
        }

        return TRUE;
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

static LRESULT CALLBACK dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        g_ctx.hwnd = hwnd;
        update_editboxes();

        for (const auto btn : button_ids)
        {
            SetWindowSubclass(GetDlgItem(hwnd, btn), hotkey_button_subclass_proc, 0, 0);
        }

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
            new_config = g_ctx.prev_config;
            EndDialog(hwnd, IDCANCEL);
            break;
        case IDC_B_CLEAR:
            new_config = t_config{};
            update_editboxes();
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

        case IDC_BAS_LEFT:
            begin_edit(IDC_EAS_LEFT, &new_config.controller_config.x);
            g_ctx.positive_target_axis = false;
            break;
        case IDC_BAS_RIGHT:
            begin_edit(IDC_EAS_RIGHT, &new_config.controller_config.x);
            g_ctx.positive_target_axis = true;
            break;
        case IDC_BAS_UP:
            begin_edit(IDC_EAS_UP, &new_config.controller_config.y);
            g_ctx.positive_target_axis = false;
            break;
        case IDC_BAS_DOWN:
            begin_edit(IDC_EAS_DOWN, &new_config.controller_config.y);
            g_ctx.positive_target_axis = true;
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

void ConfigDialog::show(HWND parent)
{
    load_config();

    g_ctx.prev_config = new_config;

    DialogBox(g_inst, MAKEINTRESOURCE(IDD_CONFIGDLG), parent, (DLGPROC)dlgproc);

    save_config();
}

void ConfigDialog::on_sdl_event(const SDL_Event &e)
{
    if (!is_editing())
    {
        return;
    }

    g_ef->log_trace(std::format(L"ConfigDialog::on_sdl_event: type={}", e.type).c_str());

    if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
    {
        if (auto *mapping = std::get_if<t_button_mapping *>(&g_ctx.target_value))
        {
            (*mapping)->button = e.gbutton.button;
            (*mapping)->key = 0;
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
                (*mapping)->key_negative = 0;
                (*mapping)->key_positive = 0;
                end_edit();
            }

            else if (axis_value > threshold)
            {
                (*mapping)->axis = e.gaxis.axis;
                (*mapping)->key_negative = 0;
                (*mapping)->key_positive = 0;
                end_edit();
            }
        }
    }
}
