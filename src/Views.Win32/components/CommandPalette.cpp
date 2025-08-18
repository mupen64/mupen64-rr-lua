/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <components/CommandPalette.h>

struct t_listbox_item {
    bool is_group_header{};
    std::wstring path{};
    std::wstring group_name{};
    std::wstring hotkey_display_name{};

    std::wstring display_name{};
    bool enabled{};
    bool active{};

    t_listbox_item() = default;
    explicit t_listbox_item(const std::wstring& action, const std::wstring& group);

    [[nodiscard]] bool selectable() const;
};

struct t_command_palette_context {
    HWND hwnd{};
    HWND listbox_hwnd{};
    HWND edit_hwnd{};
    bool closing{};
    HTHEME button_theme{};
    std::wstring search_query{};
    std::vector<std::wstring> actions{};
    std::vector<t_listbox_item> items{};
};

static t_command_palette_context g_ctx{};

t_listbox_item::t_listbox_item(const std::wstring& action, const std::wstring& group)
{
    path = action;
    group_name = group;
    const auto hotkey = g_config.hotkeys.contains(action) ? g_config.hotkeys.at(action) : Hotkey::t_hotkey{};
    if (!hotkey.is_nothing())
    {
        hotkey_display_name = hotkey.to_wstring();
    }

    display_name = ActionManager::get_display_name(action, true);
    enabled = ActionManager::get_enabled(action);
    active = ActionManager::get_active(action);
}

bool t_listbox_item::selectable() const
{
    return !is_group_header && enabled;
}

/**
 * \brief Tries to invoke the action at the specified index. Closes the command palette if successful.
 */
static bool invoke_action_at_index(int32_t i)
{
    if (i == LB_ERR || i >= ListBox_GetCount(g_ctx.listbox_hwnd))
    {
        return false;
    }

    const auto action = reinterpret_cast<t_listbox_item*>(ListBox_GetItemData(g_ctx.listbox_hwnd, i));

    if (action->is_group_header)
    {
        return false;
    }

    SendMessage(g_ctx.hwnd, WM_CLOSE, 0, 0);

    ActionManager::invoke(action->path);

    return true;
}

/**
 * \brief Finds the index of the first selectable item in the item collection.
 */
static int32_t find_index_of_first_selectable_item()
{
    int32_t i = 0;
    for (const auto& item : g_ctx.items)
    {
        if (item.selectable())
        {
            return i;
        }
        i++;
    }
    return LB_ERR;
}

/**
 * \brief Builds the action listbox based on the current search query.
 */
static void build_listbox()
{
    const auto normalize = [](std::wstring str) -> std::wstring {
        std::ranges::transform(str, str.begin(), toupper);
        str = io_service.trim(str);
        return str;
    };

    const auto action_matches_query = [&](const t_listbox_item& item, const std::wstring& query) -> bool {
        if (query.empty())
        {
            return true;
        }

        const auto normalized_action = normalize(item.display_name);
        const auto normalized_group_name = normalize(item.group_name);
        const auto normalized_hotkey = normalize(item.hotkey_display_name);

        const auto matches = normalized_action.contains(query) || normalized_group_name.contains(query) || normalized_hotkey.contains(query);

        return matches;
    };

    g_ctx.items = {};

    const auto normalized_query = normalize(g_ctx.search_query);

    std::vector<std::wstring> unique_group_names;

    for (const auto& path : g_ctx.actions)
    {
        std::vector<std::wstring> segments = ActionManager::get_segments(path);

        if (segments.size() <= 1)
        {
            continue;
        }

        segments.pop_back();

        std::wstring group_name;
        for (size_t i = 0; i < segments.size(); ++i)
        {
            if (i > 0)
            {
                group_name += ActionManager::SEGMENT_SEPARATOR;
            }
            group_name += segments[i];
        }

        if (std::ranges::find(unique_group_names, group_name) == unique_group_names.end())
        {
            unique_group_names.emplace_back(group_name);
        }
    }

    for (const auto& group : unique_group_names)
    {
        auto actions = ActionManager::get_actions_matching_filter(std::format(L"{} > *", group));

        auto segments = ActionManager::get_segments(group);
        for (auto& segment : segments)
        {
            segment = ActionManager::get_display_name(segment, true);
        }
        const auto name = io_service.join_wstring(segments, std::format(L" {} ", ActionManager::SEGMENT_SEPARATOR));

        std::erase_if(actions, [&](const auto& action) {
            const auto action_segments = ActionManager::get_segments(action);
            const auto group_segments = ActionManager::get_segments(group);

            if (action_segments.at(action_segments.size() - 2) != group_segments.back())
            {
                return true;
            }

            return !action_matches_query(t_listbox_item(action, group), normalized_query);
        });

        if (actions.empty())
        {
            continue;
        }

        t_listbox_item state{};
        state.is_group_header = true;
        state.display_name = name;
        state.enabled = true;
        g_ctx.items.push_back(state);

        for (const auto& action : actions)
        {
            g_ctx.items.emplace_back(action, group);
        }
    }

    SetWindowRedraw(g_ctx.listbox_hwnd, FALSE);
    ListBox_ResetContent(g_ctx.listbox_hwnd);
    SendMessage(g_ctx.listbox_hwnd, LB_INITSTORAGE, g_ctx.items.size(), 0);

    for (const auto& item : g_ctx.items)
    {
        ListBox_AddItemData(g_ctx.listbox_hwnd, reinterpret_cast<LPARAM>(&item));
    }

    ListBox_SetCurSel(g_ctx.listbox_hwnd, find_index_of_first_selectable_item());

    SetWindowRedraw(g_ctx.listbox_hwnd, TRUE);
}

/**
 * \brief Moves the selection in the listbox by the specified amount.
 */
static void adjust_listbox_selection(const int32_t by)
{
    const int32_t count = ListBox_GetCount(g_ctx.listbox_hwnd);
    const auto initial_index = ListBox_GetCurSel(g_ctx.listbox_hwnd);

    int32_t new_index = initial_index;

    size_t attempts = 0;
    while (true)
    {
        new_index = wrapping_clamp(new_index + by, 0, count - 1);
        attempts++;

        if (new_index == LB_ERR || new_index >= count || attempts > count)
        {
            new_index = initial_index;
            break;
        }

        const auto item = reinterpret_cast<t_listbox_item*>(ListBox_GetItemData(g_ctx.listbox_hwnd, new_index));

        if (item->selectable())
        {
            break;
        }
    }

    ListBox_SetCurSel(g_ctx.listbox_hwnd, new_index);
    listbox_ensure_visible(g_ctx.listbox_hwnd, new_index);
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
            SendMessage(g_ctx.hwnd, WM_CLOSE, 0, 0);
            return FALSE;
        }
        if (wparam == VK_UP)
        {
            adjust_listbox_selection(-1);
            return FALSE;
        }
        if (wparam == VK_DOWN)
        {
            adjust_listbox_selection(1);
            return FALSE;
        }
        if (wparam == VK_RETURN)
        {
            invoke_action_at_index(ListBox_GetCurSel(g_ctx.listbox_hwnd));
            return FALSE;
        }
        if (wparam == VK_F2)
        {
            const int32_t index = ListBox_GetCurSel(g_ctx.listbox_hwnd);
            if (index == LB_ERR)
            {
                break;
            }

            const auto item = reinterpret_cast<t_listbox_item*>(ListBox_GetItemData(g_ctx.listbox_hwnd, index));

            if (!item->selectable())
            {
                break;
            }

            Hotkey::t_hotkey hotkey = g_config.hotkeys.contains(item->path) ? g_config.hotkeys.at(item->path) : Hotkey::t_hotkey{};
            Hotkey::show_prompt(g_main_hwnd, std::format(L"Choose a hotkey for {}", item->display_name), hotkey);
            Hotkey::try_associate_hotkey(g_main_hwnd, item->path, hotkey);

            SendMessage(g_ctx.hwnd, WM_CLOSE, 0, 0);
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
            g_ctx.hwnd = hwnd;
            g_ctx.button_theme = OpenThemeData(hwnd, L"BUTTON");
            g_ctx.edit_hwnd = GetDlgItem(hwnd, IDC_COMMAND_PALETTE_EDIT);
            g_ctx.listbox_hwnd = GetDlgItem(hwnd, IDC_COMMAND_PALETTE_LIST);
            g_ctx.actions = ActionManager::get_actions_matching_filter(L"*");


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
    case WM_DESTROY:
        CloseThemeData(g_ctx.button_theme);
        break;
    case WM_CLOSE:
        g_ctx.closing = true;
        DestroyWindow(g_ctx.hwnd);
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
            case LBN_SELCHANGE:
                SetWindowRedraw(g_ctx.listbox_hwnd, FALSE);
                adjust_listbox_selection(-1);
                adjust_listbox_selection(1);
                SetWindowRedraw(g_ctx.listbox_hwnd, TRUE);
                InvalidateRect(g_ctx.listbox_hwnd, nullptr, TRUE);
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        break;
    case WM_ACTIVATE:
        if (wparam == WA_INACTIVE && !g_ctx.closing)
        {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
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
                    const auto action = reinterpret_cast<t_listbox_item*>(ListBox_GetItemData(g_ctx.listbox_hwnd, pdis->itemID));

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

                    // 1. Draw the background
                    FillRect(pdis->hDC, &pdis->rcItem, bg_brush);

                    // 2. Draw the checkbox if applicable
                    int checkbox_width = 0;
                    if (action->active)
                    {
                        SIZE checkbox_size{};
                        GetThemePartSize(g_ctx.button_theme, nullptr, BP_CHECKBOX, CBS_CHECKEDNORMAL, nullptr, TS_TRUE, &checkbox_size);
                        checkbox_width = checkbox_size.cx;

                        RECT rc = pdis->rcItem;
                        rc.left += 12;
                        rc.right = rc.left + checkbox_width;
                        rc.bottom = rc.top + checkbox_width;
                        DrawThemeBackground(g_ctx.button_theme, pdis->hDC, BP_CHECKBOX, CBS_CHECKEDNORMAL, &rc, nullptr);
                    }

                    // 3. Draw the action and hotkey text if applicable
                    SetBkMode(pdis->hDC, TRANSPARENT);
                    SetTextColor(pdis->hDC, text_color);

                    if (!action->is_group_header)
                    {
                        RECT text_rc = pdis->rcItem;
                        text_rc.left += 12;
                        if (checkbox_width > 0)
                        {
                            text_rc.left += checkbox_width + 4;
                        }

                        const auto draw_flag = action->enabled ? 0 : DSS_DISABLED;

                        DrawState(pdis->hDC, nullptr, nullptr, (LPARAM)action->display_name.c_str(), 0, text_rc.left, text_rc.top, text_rc.right - text_rc.left, text_rc.bottom - text_rc.top, draw_flag | DST_TEXT);

                        SIZE sz;
                        GetTextExtentPoint32(pdis->hDC, action->hotkey_display_name.c_str(), (int)action->hotkey_display_name.length(), &sz);
                        const int x = text_rc.right - sz.cx;

                        DrawState(pdis->hDC, nullptr, nullptr, (LPARAM)action->hotkey_display_name.c_str(), 0, x, text_rc.top, sz.cx, text_rc.bottom - text_rc.top, draw_flag | DSS_RIGHT | DST_TEXT);
                    }

                    // 4. Draw the group header if applicable
                    if (action->is_group_header)
                    {
                        const RECT text_rc = pdis->rcItem;

                        DrawState(pdis->hDC, nullptr, nullptr, (LPARAM)action->display_name.c_str(), 0, text_rc.left, text_rc.top, text_rc.right - text_rc.left, text_rc.bottom - text_rc.top, DST_TEXT);

                        HPEN pen = CreatePen(PS_DOT, 1, text_color);
                        HGDIOBJ prev_obj = SelectObject(pdis->hDC, pen);

                        SIZE sz;
                        GetTextExtentPoint32(pdis->hDC, action->display_name.c_str(), (int)action->display_name.length(), &sz);

                        MoveToEx(pdis->hDC, text_rc.left + sz.cx + 4, text_rc.top + (text_rc.bottom - text_rc.top) / 2, nullptr);
                        LineTo(pdis->hDC, text_rc.right, text_rc.top + (text_rc.bottom - text_rc.top) / 2);

                        SelectObject(pdis->hDC, prev_obj);
                    }

                    // 5. Draw the focus rect
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
    const HWND hwnd = CreateDialog(g_app_instance, MAKEINTRESOURCE(IDD_COMMAND_PALETTE), g_main_hwnd, command_palette_proc);
    ShowWindow(hwnd, SW_SHOW);
}
