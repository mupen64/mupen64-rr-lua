/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "HotkeyUtils.hpp"

struct DialogParams
{
    std::wstring headline{};
    Hotkey hotkey = Hotkey::make_unassigned();
};

static LRESULT CALLBACK HotkeyButtonSubclassProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id,
                                                 DWORD_PTR ref_data)
{
    const auto params = reinterpret_cast<DialogParams *>(ref_data);

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, HotkeyButtonSubclassProc, id);
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

static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    const auto prop_key = L"IDD_HOTKEY_Params";
    auto params = static_cast<DialogParams *>(GetProp(hwnd, prop_key));

    switch (msg)
    {
    case WM_INITDIALOG: {
        SetProp(hwnd, prop_key, reinterpret_cast<DialogParams *>(lparam));
        params = reinterpret_cast<DialogParams *>(lparam);

        Static_SetText(GetDlgItem(hwnd, IDC_STATIC), params->headline.c_str());
        SetFocus(GetDlgItem(hwnd, IDC_CURRENT_HOTKEY));

        SetWindowSubclass(GetDlgItem(hwnd, IDC_CURRENT_HOTKEY), HotkeyButtonSubclassProc, 0,
                          reinterpret_cast<DWORD_PTR>(params));
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
            params->hotkey = Hotkey::make_empty();
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

bool HotkeyUtils::show_prompt(const HWND hwnd, const std::wstring &caption, Hotkey &hotkey)
{
    const auto prev_hotkey = hotkey;

    hotkey = Hotkey::make_unassigned();
    auto params = new DialogParams{.headline = caption, .hotkey = hotkey};

    const INT_PTR result =
        DialogBoxParam(g_main_ctx.hinst, MAKEINTRESOURCE(IDD_HOTKEY), hwnd, DlgProc, reinterpret_cast<LPARAM>(params));
    const bool confirmed = result == IDOK;

    if (confirmed)
    {
        hotkey = params->hotkey;
    }
    else
    {
        hotkey = prev_hotkey;
    }
    hotkey.assigned = true;

    delete params;

    return confirmed;
}

void HotkeyUtils::try_associate_hotkey(const HWND hwnd, const std::wstring &action, const Hotkey &new_hotkey,
                                  const bool through_action_manager)
{
    const auto set_hotkey = [=](const std::wstring &action, const Hotkey &hotkey) {
        if (through_action_manager)
        {
            ActionManager::associate_hotkey(action, hotkey);
        }
        else
        {
            g_config.hotkeys[action] = hotkey;
        }
    };

    if (new_hotkey.is_empty())
    {
        set_hotkey(action, Hotkey::make_empty());
        return;
    }

    if (g_config.hotkeys.at(action) == new_hotkey)
    {
        return;
    }

    std::vector<std::pair<std::wstring, Hotkey>> conflicting_hotkeys;

    for (const auto &pair : g_config.hotkeys)
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
    for (const auto &action : conflicting_hotkeys | std::views::keys)
    {
        conflicting_hotkey_identifiers += std::format(L"- {}\n", action);
    }

    const auto str = std::format(L"The key combination {} is already used by:\n\n{}\nHow would you like to proceed?",
                                 new_hotkey.to_wstring(), conflicting_hotkey_identifiers);

    const size_t choice = DialogService::show_multiple_choice_dialog(
        VIEW_DLG_HOTKEY_CONFLICT, {L"Keep New", L"Keep Old", L"Proceed Anyway"}, str.c_str(), L"Hotkey Conflict",
        fsvc_warning, hwnd);

    switch (choice)
    {
    case 0:
        for (const auto &action : conflicting_hotkeys | std::views::keys)
        {
            set_hotkey(action, Hotkey::make_empty());
        }
        set_hotkey(action, new_hotkey);
        break;
    case 1:
        set_hotkey(action, Hotkey::make_empty());
        break;
    case 2:
        set_hotkey(action, new_hotkey);
        break;
    default:
        break;
    }
}
