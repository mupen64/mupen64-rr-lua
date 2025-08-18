/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Hotkey.h>

struct t_hotkey_dialog_params {
    std::wstring headline{};
    Hotkey::t_hotkey hotkey{};
};

bool Hotkey::t_hotkey::is_nothing() const
{
    return !this->ctrl && !this->shift && !this->alt && this->key == 0;
}

std::wstring Hotkey::t_hotkey::to_wstring() const
{
    wchar_t buf[260]{};
    const int k = this->key;

    if (!this->ctrl && !this->shift && !this->alt && !this->key)
    {
        return L"(nothing)";
    }

    if (this->ctrl)
        StrCat(buf, L"Ctrl ");
    if (this->shift)
        StrCat(buf, L"Shift ");
    if (this->alt)
        StrCat(buf, L"Alt ");
    if (k)
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
                StrCpy(buf2, L"LMB");
                break;
            case VK_RBUTTON:
                StrCpy(buf2, L"RMB");
                break;
            case VK_MBUTTON:
                StrCpy(buf2, L"MMB");
                break;
            case VK_XBUTTON1:
                StrCpy(buf2, L"XMB1");
                break;
            case VK_XBUTTON2:
                StrCpy(buf2, L"XMB2");
                break;
            case VK_SPACE:
                StrCpy(buf2, L"Space");
                break;
            case VK_BACK:
                StrCpy(buf2, L"Backspace");
                break;
            case VK_TAB:
                StrCpy(buf2, L"Tab");
                break;
            case VK_CLEAR:
                StrCpy(buf2, L"Clear");
                break;
            case VK_RETURN:
                StrCpy(buf2, L"Enter");
                break;
            case VK_PAUSE:
                StrCpy(buf2, L"Pause");
                break;
            case VK_CAPITAL:
                StrCpy(buf2, L"Caps");
                break;
            case VK_PRIOR:
                StrCpy(buf2, L"PageUp");
                break;
            case VK_NEXT:
                StrCpy(buf2, L"PageDn");
                break;
            case VK_END:
                StrCpy(buf2, L"End");
                break;
            case VK_HOME:
                StrCpy(buf2, L"Home");
                break;
            case VK_LEFT:
                StrCpy(buf2, L"Left");
                break;
            case VK_UP:
                StrCpy(buf2, L"Up");
                break;
            case VK_RIGHT:
                StrCpy(buf2, L"Right");
                break;
            case VK_DOWN:
                StrCpy(buf2, L"Down");
                break;
            case VK_SELECT:
                StrCpy(buf2, L"Select");
                break;
            case VK_PRINT:
                StrCpy(buf2, L"Print");
                break;
            case VK_SNAPSHOT:
                StrCpy(buf2, L"PrintScrn");
                break;
            case VK_INSERT:
                StrCpy(buf2, L"Insert");
                break;
            case VK_DELETE:
                StrCpy(buf2, L"Delete");
                break;
            case VK_HELP:
                StrCpy(buf2, L"Help");
                break;
            case VK_MULTIPLY:
                StrCpy(buf2, L"Num*");
                break;
            case VK_ADD:
                StrCpy(buf2, L"Num+");
                break;
            case VK_SUBTRACT:
                StrCpy(buf2, L"Num-");
                break;
            case VK_DECIMAL:
                StrCpy(buf2, L"Num.");
                break;
            case VK_DIVIDE:
                StrCpy(buf2, L"Num/");
                break;
            case VK_NUMLOCK:
                StrCpy(buf2, L"NumLock");
                break;
            case VK_SCROLL:
                StrCpy(buf2, L"ScrollLock");
                break;
            case /*VK_OEM_PLUS*/ 0xBB:
                StrCpy(buf2, L"=+");
                break;
            case /*VK_OEM_MINUS*/ 0xBD:
                StrCpy(buf2, L"-_");
                break;
            case /*VK_OEM_COMMA*/ 0xBC:
                StrCpy(buf2, L",");
                break;
            case /*VK_OEM_PERIOD*/ 0xBE:
                StrCpy(buf2, L".");
                break;
            case VK_OEM_7:
                StrCpy(buf2, L"'\"");
                break;
            case VK_OEM_6:
                StrCpy(buf2, L"]}");
                break;
            case VK_OEM_5:
                StrCpy(buf2, L"\\|");
                break;
            case VK_OEM_4:
                StrCpy(buf2, L"[{");
                break;
            case VK_OEM_3:
                StrCpy(buf2, L"`~");
                break;
            case VK_OEM_2:
                StrCpy(buf2, L"/?");
                break;
            case VK_OEM_1:
                StrCpy(buf2, L";:");
                break;
            default:
                wsprintf(buf2, L"(%d)", k);
                break;
            }
        StrCat(buf, buf2);
    }
    return buf;
}

static LRESULT CALLBACK hotkey_button_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR ref_data)
{
    const auto params = reinterpret_cast<t_hotkey_dialog_params*>(ref_data);

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
        if (wparam == VK_CONTROL)
        {
            params->hotkey.ctrl = true;
        }
        else if (wparam == VK_SHIFT)
        {
            params->hotkey.shift = true;
        }
        else if (wparam == VK_MENU)
        {
            params->hotkey.alt = true;
        }
        else
        {
            params->hotkey.key = wparam;
            EndDialog(GetParent(hwnd), IDOK);
        }

        SetDlgItemText(GetParent(hwnd), IDC_CURRENT_HOTKEY, params->hotkey.to_wstring().c_str());

        return TRUE;
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}
static INT_PTR CALLBACK dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    const auto prop_key = L"IDD_HOTKEY_Params";
    auto params = static_cast<t_hotkey_dialog_params*>(GetProp(hwnd, prop_key));

    switch (msg)
    {
    case WM_INITDIALOG:
        {
            SetProp(hwnd, prop_key, reinterpret_cast<t_hotkey_dialog_params*>(lparam));
            params = reinterpret_cast<t_hotkey_dialog_params*>(lparam);

            Static_SetText(GetDlgItem(hwnd, IDC_STATIC), params->headline.c_str());
            SetFocus(GetDlgItem(hwnd, IDC_CURRENT_HOTKEY));

            SetWindowSubclass(GetDlgItem(hwnd, IDC_CURRENT_HOTKEY), hotkey_button_subclass_proc, 0, reinterpret_cast<DWORD_PTR>(params));
            return TRUE;
        }
    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDOK:
            EndDialog(hwnd, IDOK);
            return TRUE;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        case IDC_CLEAR:
            params->hotkey = {};
            EndDialog(hwnd, IDOK);
            break;
        default:
            break;
        }
        break;
    case WM_MBUTTONDOWN:
        params->hotkey.key = VK_MBUTTON;
        EndDialog(hwnd, IDOK);
        break;
    case WM_XBUTTONDOWN:
        if (HIWORD(wparam) == XBUTTON1)
        {
            params->hotkey.key = VK_XBUTTON1;
            EndDialog(hwnd, IDOK);
        }
        if (HIWORD(wparam) == XBUTTON2)
        {
            params->hotkey.key = VK_XBUTTON2;
            EndDialog(hwnd, IDOK);
        }
        break;
    default:
        break;
    }
    return FALSE;
}

bool Hotkey::show_prompt(const HWND hwnd, const std::wstring& caption, t_hotkey& hotkey)
{
    const auto prev_hotkey = hotkey;

    hotkey = {};
    auto params = new t_hotkey_dialog_params{.headline = caption, .hotkey = hotkey};

    const INT_PTR result = DialogBoxParam(g_app_instance, MAKEINTRESOURCE(IDD_HOTKEY), hwnd, dlgproc, reinterpret_cast<LPARAM>(params));
    const bool confirmed = result == IDOK;

    if (confirmed)
    {
        hotkey = params->hotkey;
    }
    else
    {
        hotkey = prev_hotkey;
    }

    delete params;

    return confirmed;
}

void Hotkey::try_associate_hotkey(const HWND hwnd, const std::wstring& action, const t_hotkey& new_hotkey, const bool through_action_manager)
{
    const auto set_hotkey = [=](const std::wstring& action, const t_hotkey& hotkey) {
        if (through_action_manager)
        {
            ActionManager::associate_hotkey(action, hotkey);
        }
        else
        {
            g_config.hotkeys[action] = hotkey;
        }
    };

    if (new_hotkey.is_nothing())
    {
        set_hotkey(action, {});
        return;
    }

    if (g_config.hotkeys[action] == new_hotkey)
    {
        return;
    }

    std::vector<std::pair<std::wstring, t_hotkey>> conflicting_hotkeys;

    for (const auto& pair : g_config.hotkeys)
    {
        if (pair.first != action && pair.second == new_hotkey)
        {
            conflicting_hotkeys.emplace_back(pair);
        }
    }

    if (conflicting_hotkeys.empty())
    {
        set_hotkey(action, new_hotkey);
        return;
    }

    std::wstring conflicting_hotkey_identifiers;
    for (const auto& action : conflicting_hotkeys | std::views::keys)
    {
        conflicting_hotkey_identifiers += std::format(L"- {}\n", action);
    }

    const auto str = std::format(L"The key combination {} is already used by:\n\n{}\nHow would you like to proceed?",
                                 new_hotkey.to_wstring(),
                                 conflicting_hotkey_identifiers);

    const size_t choice = DialogService::show_multiple_choice_dialog(VIEW_DLG_HOTKEY_CONFLICT, {L"Keep New", L"Keep Old", L"Proceed Anyway"}, str.c_str(), L"Hotkey Conflict", fsvc_warning, hwnd);

    switch (choice)
    {
    case 0:
        for (const auto& action : conflicting_hotkeys | std::views::keys)
        {
            set_hotkey(action, {});
        }
        set_hotkey(action, new_hotkey);
        break;
    case 1:
        set_hotkey(action, {});
        break;
    case 2:
        set_hotkey(action, new_hotkey);
        break;
    default:
        break;
    }
}
