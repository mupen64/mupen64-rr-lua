/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <components/CommandPalette.h>

struct t_action_stored_state {
    std::wstring path{};
    std::wstring display_name{};
    std::wstring hotkey_display_name{};
    bool enabled{};
    bool active{};
};

struct t_command_palette_context {
    HWND hwnd{};
    HWND listbox_hwnd{};
    HWND edit_hwnd{};
    std::wstring search_query{};
    std::vector<t_action_stored_state> actions{};
};

static t_command_palette_context g_ctx{};

/**
 * \brief Tries to invoke the action at the specified index.
 */
static bool invoke_action_at_index(int32_t i)
{
    if (i == LB_ERR || i >= ListBox_GetCount(g_ctx.listbox_hwnd))
    {
        return false;
    }

    const auto action = reinterpret_cast<t_action_stored_state*>(ListBox_GetItemData(g_ctx.listbox_hwnd, i));
    ActionManager::invoke(action->path);

    return true;
}

/**
 * \brief Builds the action listbox based on the current search query.
 */
static void build_listbox()
{
    const auto prev_selected_index = ListBox_GetCurSel(g_ctx.listbox_hwnd);

    const auto normalize = [](std::wstring str) -> std::wstring {
        std::ranges::transform(str, str.begin(), toupper);
        str = io_service.trim(str);
        return str;
    };

    const auto normalized_query = normalize(g_ctx.search_query);

    SetWindowRedraw(g_ctx.listbox_hwnd, FALSE);
    ListBox_ResetContent(g_ctx.listbox_hwnd);
    SendMessage(g_ctx.listbox_hwnd, LB_INITSTORAGE, g_ctx.actions.size(), (LPARAM)(g_ctx.actions.size() * 30 * sizeof(wchar_t)));

    for (const auto& action : g_ctx.actions)
    {
        const auto normalized_action = normalize(action.display_name);
        const auto normalized_hotkey = normalize(action.hotkey_display_name);

        const auto matches = normalized_action.contains(normalized_query) || normalized_hotkey.contains(normalized_query);

        if (!normalized_query.empty() && !matches)
        {
            continue;
        }

        ListBox_AddItemData(g_ctx.listbox_hwnd, reinterpret_cast<LPARAM>(&action));
    }


    if (ListBox_GetCount(g_ctx.listbox_hwnd) > 0)
    {
        ListBox_SetCurSel(g_ctx.listbox_hwnd, 0);
    }
    
    SetWindowRedraw(g_ctx.listbox_hwnd, TRUE);
}

static LRESULT CALLBACK keyboard_interaction_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR ref_data)
{
    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, keyboard_interaction_subclass_proc, id);
        break;
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wparam == VK_ESCAPE)
        {
            EndDialog(GetParent(hwnd), IDCANCEL);
            return FALSE;
        }
        if (wparam == VK_UP)
        {
            const auto selected_index = ListBox_GetCurSel(g_ctx.listbox_hwnd);
            const auto count = ListBox_GetCount(g_ctx.listbox_hwnd);
            const auto new_index = wrapping_clamp(selected_index - 1, 0, count - 1);
            ListBox_SetCurSel(g_ctx.listbox_hwnd, new_index);
            ListBox_SetTopIndex(g_ctx.listbox_hwnd, new_index);
            return FALSE;
        }
        if (wparam == VK_DOWN)
        {
            const auto selected_index = ListBox_GetCurSel(g_ctx.listbox_hwnd);
            const auto count = ListBox_GetCount(g_ctx.listbox_hwnd);
            const auto new_index = wrapping_clamp(selected_index + 1, 0, count - 1);
            ListBox_SetCurSel(g_ctx.listbox_hwnd, new_index);
            ListBox_SetTopIndex(g_ctx.listbox_hwnd, new_index);
            return FALSE;
        }
        if (wparam == VK_RETURN)
        {
            if (invoke_action_at_index(ListBox_GetCurSel(g_ctx.listbox_hwnd)))
            {
                EndDialog(GetParent(hwnd), IDOK);
            }

            return FALSE;
        }
        break;
    default:
        break;
    }
    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

static INT_PTR CALLBACK command_palette_proc(const HWND hwnd, const UINT msg, const WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            g_ctx.edit_hwnd = GetDlgItem(hwnd, IDC_COMMAND_PALETTE_EDIT);
            g_ctx.listbox_hwnd = GetDlgItem(hwnd, IDC_COMMAND_PALETTE_LIST);
            const auto actions = ActionManager::get_actions_matching_filter(L"*");

            g_ctx.actions.reserve(actions.size());
            for (const auto& action : actions)
            {
                t_action_stored_state state{};
                state.path = action;
                state.display_name = ActionManager::get_display_name(action, true);
                state.enabled = ActionManager::get_enabled(action);
                state.active = ActionManager::get_active(action);

                const auto hotkey = g_config.hotkeys.contains(action) ? g_config.hotkeys.at(action) : Hotkey::t_hotkey{};
                if (!hotkey.is_nothing())
                {
                    state.hotkey_display_name = hotkey.to_wstring();
                }

                g_ctx.actions.push_back(state);
            }


            // 1. Remove the titlebar
            const LONG style = GetWindowLong(hwnd, GWL_STYLE);
            SetWindowLong(hwnd, GWL_STYLE, style & ~WS_CAPTION);

            // 2. Add resize anchors
            ResizeAnchor::add_anchors(hwnd,
                                      {
                                      {g_ctx.edit_hwnd, ResizeAnchor::HORIZONTAL_ANCHOR},
                                      {g_ctx.listbox_hwnd, ResizeAnchor::FULL_ANCHOR},
                                      });

            // 3. Set a reasonable position and size for the dialog (centered horizontally, vertically top-justified)
            RECT parent_rc{};
            GetClientRect(g_main_hwnd, &parent_rc);

            constexpr auto margin = 10;
            const auto width = std::max(400L, parent_rc.right / 3 - margin);
            const auto height = std::max(100L, parent_rc.bottom / 2 - margin);

            RECT rc;
            rc.left = parent_rc.right / 2 - width / 2;
            rc.top = margin;
            rc.right = rc.left + width;
            rc.bottom = rc.top + height;

            MapWindowRect(g_main_hwnd, HWND_DESKTOP, &rc);
            SetWindowPos(hwnd, nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_FRAMECHANGED);

            // 4. Build the listbox
            build_listbox();

            // 5. Subclass the controls for key event handling
            SetWindowSubclass(g_ctx.edit_hwnd, keyboard_interaction_subclass_proc, 0, 0);
            SetWindowSubclass(g_ctx.listbox_hwnd, keyboard_interaction_subclass_proc, 0, 0);

            // 6. Set the focus to the edit control
            SetFocus(g_ctx.edit_hwnd);

            break;
        }
    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        break;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_COMMAND_PALETTE_EDIT:
            switch (HIWORD(wparam))
            {
            case EN_CHANGE:
                {
                    wchar_t text[80]{};
                    Edit_GetText(g_ctx.edit_hwnd, text, std::size(text));

                    if (g_ctx.search_query != text)
                    {
                        g_ctx.search_query = text;
                        build_listbox();
                    }
                    break;
                }
            default:
                break;
            }
        case IDC_COMMAND_PALETTE_LIST:
            switch (HIWORD(wparam))
            {
            case LBN_DBLCLK:
                if (invoke_action_at_index(ListBox_GetCurSel(g_ctx.listbox_hwnd)))
                {
                    EndDialog(hwnd, IDOK);
                }
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        break;
    case WM_MEASUREITEM:
        {
            const auto pmis = (PMEASUREITEMSTRUCT)lparam;
            pmis->itemHeight = 18;
            return TRUE;
        }
    case WM_DRAWITEM:
        {
            const auto pdis = reinterpret_cast<PDRAWITEMSTRUCT>(lparam);

            if (std::cmp_equal(pdis->itemID, -1))
            {
                break;
            }

            switch (pdis->itemAction)
            {
            case ODA_SELECT:
            case ODA_DRAWENTIRE:
                {
                    const auto action = reinterpret_cast<t_action_stored_state*>(ListBox_GetItemData(g_ctx.listbox_hwnd, pdis->itemID));

                    COLORREF text_color;
                    HBRUSH bg_brush;

                    if (pdis->itemState & ODS_SELECTED)
                    {
                        text_color = GetSysColor(COLOR_HIGHLIGHTTEXT);
                        bg_brush = GetSysColorBrush(COLOR_HIGHLIGHT);
                    }
                    else
                    {
                        text_color = GetSysColor(COLOR_WINDOWTEXT);
                        bg_brush = GetSysColorBrush(COLOR_WINDOW);
                    }

                    FillRect(pdis->hDC, &pdis->rcItem, bg_brush);

                    SetBkMode(pdis->hDC, TRANSPARENT);
                    SetTextColor(pdis->hDC, text_color);

                    const auto draw_flag = action->enabled ? 0 : DSS_DISABLED;

                    DrawState(pdis->hDC, nullptr, nullptr, (LPARAM)action->display_name.c_str(), 0, pdis->rcItem.left, pdis->rcItem.top, pdis->rcItem.right - pdis->rcItem.left, pdis->rcItem.bottom - pdis->rcItem.top, draw_flag | DST_TEXT);

                    SIZE sz;
                    GetTextExtentPoint32(pdis->hDC, action->hotkey_display_name.c_str(), (int)action->hotkey_display_name.length(), &sz);
                    const int x = pdis->rcItem.right - sz.cx;

                    DrawState(pdis->hDC, nullptr, nullptr, (LPARAM)action->hotkey_display_name.c_str(), 0, x, pdis->rcItem.top, sz.cx, pdis->rcItem.bottom - pdis->rcItem.top, draw_flag | DSS_RIGHT | DST_TEXT);

                    if (pdis->itemState & ODS_FOCUS)
                    {
                        DrawFocusRect(pdis->hDC, &pdis->rcItem);
                    }

                    break;
                }
            default:
                break;
            }
            return TRUE;
        }
    default:
        return FALSE;
    }
    return TRUE;
}

void CommandPalette::show()
{
    g_ctx = {};
    DialogBox(g_app_instance, MAKEINTRESOURCE(IDD_COMMAND_PALETTE), g_main_hwnd, command_palette_proc);
}
