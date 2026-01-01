/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "RomBrowser.h"
#include "stdafx.h"
#include <components/CommandPalette.h>
#include <components/ConfigDialog.h>

struct t_listbox_item
{
    bool is_group{};
    std::wstring parent_group_name{};

    std::wstring text{};
    std::wstring hint_text{};

    bool enabled{};
    bool active{};
    bool activatable{};

    // (only applicable if action)
    std::wstring path{};
    std::wstring raw_display_name{};

    // (only applicable if option)
    ConfigDialog::t_options_item *option_item{};

    static t_listbox_item make_group(const std::wstring &group_name);
    static t_listbox_item make_action(const std::wstring &action, const std::wstring &group);
    static t_listbox_item make_option(ConfigDialog::t_options_item *item, const ConfigDialog::t_options_group &group);
    static t_listbox_item make_option_group(const ConfigDialog::t_options_group &options_group);
    static t_listbox_item make_rom(const std::wstring &name);

    [[nodiscard]] bool selectable() const;

    /**
     * \brief Determines whether the given action item matches the search query.
     */
    [[nodiscard]] bool matches_query(const std::wstring_view query) const;

  private:
    t_listbox_item() = default;
};

struct t_command_palette_context
{
    HWND hwnd{};
    HWND text_hwnd{};
    HWND listbox_hwnd{};
    HWND edit_hwnd{};
    bool closing{};
    bool dont_close_on_focus_loss{};
    HTHEME button_theme{};
    std::wstring search_query{};
    std::vector<std::wstring> actions{};
    std::vector<t_listbox_item> items{};
    std::vector<ConfigDialog::t_options_group> option_groups{};
};

static t_command_palette_context g_ctx{};

/**
 * \brief Normalizes a string for comparison.
 */
static std::wstring normalize(std::wstring str)
{
    std::ranges::transform(str, str.begin(), toupper);
    str = MiscHelpers::trim(str);
    return str;
}

t_listbox_item t_listbox_item::make_group(const std::wstring &group_name)
{
    t_listbox_item item{};
    item.is_group = true;
    item.text = group_name;
    item.enabled = true;
    return item;
}

t_listbox_item t_listbox_item::make_action(const std::wstring &action, const std::wstring &group)
{
    t_listbox_item item{};

    item.path = action;
    item.parent_group_name = group;
    const auto hotkey = g_config.hotkeys.at(action);
    if (!hotkey.is_empty())
    {
        item.hint_text = hotkey.to_wstring();
    }

    item.text = ActionManager::get_display_name(action);
    item.raw_display_name = ActionManager::get_display_name(action, true);
    item.enabled = ActionManager::get_enabled(action);
    item.active = ActionManager::get_active(action);
    item.activatable = ActionManager::get_activatability(action);
    return item;
}

t_listbox_item t_listbox_item::make_option(ConfigDialog::t_options_item *options_item,
                                           const ConfigDialog::t_options_group &group)
{
    t_listbox_item item{};
    item.text = options_item->get_name();
    item.parent_group_name = group.name;
    item.hint_text = options_item->get_value_name();
    item.enabled = !options_item->is_readonly();
    item.active = false;
    item.activatable = false;
    item.option_item = options_item;

    if (options_item->type == ConfigDialog::t_options_item::Type::Bool)
    {
        item.active = std::get<int32_t>(options_item->current_value.get()) != 0;
        item.activatable = true;
        item.hint_text = L"";
    }
    return item;
}

t_listbox_item t_listbox_item::make_option_group(const ConfigDialog::t_options_group &options_group)
{
    t_listbox_item item{};
    item.is_group = true;
    item.text = options_group.name;
    item.enabled = true;
    return item;
}

t_listbox_item t_listbox_item::make_rom(const std::wstring &name)
{
    t_listbox_item item{};
    item.parent_group_name = L"ROMs";
    item.text = name;
    item.enabled = true;
    return item;
}

bool t_listbox_item::selectable() const
{
    return !is_group && enabled;
}

bool t_listbox_item::matches_query(const std::wstring_view query) const
{
    if (query.empty())
    {
        return true;
    }

    const auto normalized_action = normalize(text);
    const auto normalized_group_name = normalize(parent_group_name);
    const auto normalized_hotkey = normalize(hint_text);
    const auto normalized_raw_display_name = normalize(raw_display_name);

    const auto matches = normalized_action.contains(query) || normalized_group_name.contains(query) ||
                         normalized_hotkey.contains(query) || normalized_raw_display_name.contains(query);

    return matches;
}

/**
 * \brief Tries to invoke the item at the specified index. Closes the command palette if successful.
 */
static bool try_invoke(int32_t i)
{
    if (i == LB_ERR || i >= ListBox_GetCount(g_ctx.listbox_hwnd))
    {
        return false;
    }

    const auto action = reinterpret_cast<t_listbox_item *>(ListBox_GetItemData(g_ctx.listbox_hwnd, i));

    if (action->is_group)
    {
        return false;
    }

    if (!action->path.empty())
    {
        SendMessage(g_ctx.hwnd, WM_CLOSE, 0, 0);
        ActionManager::invoke(action->path);
        return true;
    }

    if (action->option_item)
    {
        // HACK: We want to keep the command palette open (in case the user cancels and wants to keep looking through
        // the command palette) while editing the option, but we also want to prevent it from closing
        EnableWindow(g_ctx.hwnd, false);
        g_ctx.dont_close_on_focus_loss = true;
        const auto confirmed = action->option_item->edit(g_ctx.hwnd);
        g_ctx.dont_close_on_focus_loss = false;
        EnableWindow(g_ctx.hwnd, true);

        if (confirmed)
        {
            Config::apply_and_save();
            SendMessage(g_ctx.hwnd, WM_CLOSE, 0, 0);
            return true;
        }
        return false;
    }

    return true;
}

/**
 * \brief Tries to change the hotkey of the item at the specified index. Closes the command palette if successful.
 */
static bool try_change_hotkey(int32_t i)
{
    if (i == LB_ERR || i >= ListBox_GetCount(g_ctx.listbox_hwnd))
    {
        return false;
    }

    const auto item = reinterpret_cast<t_listbox_item *>(ListBox_GetItemData(g_ctx.listbox_hwnd, i));

    if (!item->selectable())
    {
        return false;
    }

    if (item->path.empty())
    {
        return false;
    }

    Hotkey::t_hotkey hotkey = g_config.hotkeys.at(item->path);
    Hotkey::show_prompt(g_main_ctx.hwnd, std::format(L"Choose a hotkey for {}", item->text), hotkey);
    Hotkey::try_associate_hotkey(g_main_ctx.hwnd, item->path, hotkey);

    return true;
}

/**
 * \brief Finds the index of the first selectable item in the item collection.
 */
static int32_t find_index_of_first_selectable_item()
{
    int32_t i = 0;
    for (const auto &item : g_ctx.items)
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
 * \brief Adds actions to the listbox item collection.
 */
static void add_actions(const std::wstring_view query)
{
    // 1. Collect groups
    std::vector<std::wstring> unique_group_names;

    for (const auto &path : g_ctx.actions)
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

    // 2. For each group, add matching actions
    for (const auto &group : unique_group_names)
    {
        auto actions = ActionManager::get_actions_matching_filter(std::format(L"{} > *", group));

        auto segments = ActionManager::get_segments(group);
        for (auto &segment : segments)
        {
            segment = ActionManager::get_display_name(segment, true);
        }
        const auto name = MiscHelpers::join_wstring(segments, std::format(L" {} ", ActionManager::SEGMENT_SEPARATOR));

        std::erase_if(actions, [&](const auto &action) {
            const auto action_segments = ActionManager::get_segments(action);
            const auto group_segments = ActionManager::get_segments(group);

            if (action_segments.at(action_segments.size() - 2) != group_segments.back())
            {
                return true;
            }

            return !t_listbox_item::make_action(action, group).matches_query(query);
        });

        if (actions.empty())
        {
            continue;
        }

        g_ctx.items.emplace_back(t_listbox_item::make_group(name));

        for (const auto &action : actions)
        {
            g_ctx.items.emplace_back(t_listbox_item::make_action(action, group));
        }
    }
}

/**
 * \brief Adds configuration options to the listbox item collection.
 */
static void add_options(const std::wstring_view query)
{
    g_ctx.option_groups = ConfigDialog::get_option_groups();

    for (auto &group : g_ctx.option_groups)
    {
        std::erase_if(group.items, [&](ConfigDialog::t_options_item &item) {
            return item.type == ConfigDialog::t_options_item::Type::Hotkey ||
                   !t_listbox_item::make_option(&item, group).matches_query(query);
        });

        if (group.items.empty())
        {
            continue;
        }

        g_ctx.items.emplace_back(t_listbox_item::make_option_group(group));

        for (auto &item : group.items)
        {
            g_ctx.items.emplace_back(t_listbox_item::make_option(&item, group));
        }
    }
}

/**
 * \brief Adds known ROMs to the listbox item collection.
 */
static void add_roms(const std::wstring_view query)
{
    g_ctx.items.emplace_back(t_listbox_item::make_group(L"ROMs"));

    const auto roms = RomBrowser::get_discovered_roms();

    for (const auto &rom : roms)
    {
        const auto name = std::filesystem::path(rom.path).filename().wstring();

        const auto item = t_listbox_item::make_rom(name);
        if (item.matches_query(query))
        {
            g_ctx.items.emplace_back(item);
        }
    }
}

/**
 * \brief Builds the action listbox based on the current search query.
 */
static void build_listbox()
{
    g_ctx.items = {};

    const auto normalized_query = normalize(g_ctx.search_query);

    add_actions(normalized_query);
    add_options(normalized_query);
    add_roms(normalized_query);

    SetWindowRedraw(g_ctx.listbox_hwnd, FALSE);
    ListBox_ResetContent(g_ctx.listbox_hwnd);
    SendMessage(g_ctx.listbox_hwnd, LB_INITSTORAGE, g_ctx.items.size(), 0);

    for (const auto &item : g_ctx.items)
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
        new_index = MiscHelpers::wrapping_clamp(new_index + by, 0, count - 1);
        attempts++;

        if (new_index == LB_ERR || new_index >= count || attempts > count)
        {
            new_index = initial_index;
            break;
        }

        const auto item = reinterpret_cast<t_listbox_item *>(ListBox_GetItemData(g_ctx.listbox_hwnd, new_index));

        if (item->selectable())
        {
            break;
        }
    }

    ListBox_SetCurSel(g_ctx.listbox_hwnd, new_index);
    listbox_ensure_visible(g_ctx.listbox_hwnd, new_index);
}
static LRESULT CALLBACK keyboard_interaction_subclass_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                                           UINT_PTR id, DWORD_PTR ref_data)
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
            try_invoke(ListBox_GetCurSel(g_ctx.listbox_hwnd));
            return FALSE;
        }
        if (wparam == VK_F2)
        {
            const auto selected_index = ListBox_GetCurSel(g_ctx.listbox_hwnd);

            // HACK: We want to keep the command palette open while changing the hotkey, but we also want to prevent it
            // from closing.
            EnableWindow(g_ctx.hwnd, false);
            g_ctx.dont_close_on_focus_loss = true;
            try_change_hotkey(selected_index);
            EnableWindow(g_ctx.hwnd, true);
            g_ctx.dont_close_on_focus_loss = false;

            SetFocus(g_ctx.edit_hwnd);
            build_listbox();

            // Advance selection to next item.
            ListBox_SetCurSel(g_ctx.listbox_hwnd, selected_index + 1);

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
    case WM_INITDIALOG: {
        g_ctx.hwnd = hwnd;
        g_ctx.button_theme = OpenThemeData(hwnd, L"BUTTON");
        g_ctx.text_hwnd = GetDlgItem(hwnd, IDC_COMMAND_PALETTE_TEXT);
        g_ctx.edit_hwnd = GetDlgItem(hwnd, IDC_COMMAND_PALETTE_EDIT);
        g_ctx.listbox_hwnd = GetDlgItem(hwnd, IDC_COMMAND_PALETTE_LIST);
        g_ctx.actions = ActionManager::get_actions_matching_filter(L"*");

        // 1. Remove the titlebar
        const LONG style = GetWindowLong(hwnd, GWL_STYLE);
        SetWindowLong(hwnd, GWL_STYLE, style & ~WS_CAPTION);

        // 2. Add resize anchors
        ResizeAnchor::add_anchors(hwnd, {
                                            {g_ctx.text_hwnd, ResizeAnchor::HORIZONTAL_ANCHOR},
                                            {g_ctx.edit_hwnd, ResizeAnchor::HORIZONTAL_ANCHOR},
                                            {g_ctx.listbox_hwnd, ResizeAnchor::FULL_ANCHOR},
                                        });

        // 3. Set a reasonable position and size for the dialog (centered horizontally, vertically top-justified)
        RECT parent_rc{};
        GetClientRect(g_main_ctx.hwnd, &parent_rc);

        constexpr auto margin = 10;
        const auto width = std::max(400L, parent_rc.right / 3 - margin);
        const auto height = std::max(100L, parent_rc.bottom / 2 - margin);

        RECT rc;
        rc.left = parent_rc.right / 2 - width / 2;
        rc.top = margin;
        rc.right = rc.left + width;
        rc.bottom = rc.top + height;

        MapWindowRect(g_main_ctx.hwnd, HWND_DESKTOP, &rc);
        SetWindowPos(hwnd, nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                     SWP_NOZORDER | SWP_FRAMECHANGED);

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
            case EN_CHANGE: {
                const auto query = get_window_text(g_ctx.edit_hwnd).value();
                if (g_ctx.search_query != query)
                {
                    g_ctx.search_query = query;
                    build_listbox();
                }
                break;
            }
            default:
                break;
            }
            break;
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
        if (wparam == WA_INACTIVE && !g_ctx.closing && !g_ctx.dont_close_on_focus_loss)
        {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
        break;
    case WM_MEASUREITEM: {
        const auto pmis = (PMEASUREITEMSTRUCT)lparam;
        const auto scale = (double)GetDpiForWindow(hwnd) / 96.0;
        pmis->itemHeight = (UINT)(14.0 * scale);
        return TRUE;
    }
    case WM_DRAWITEM: {
        const auto pdis = reinterpret_cast<PDRAWITEMSTRUCT>(lparam);

        if (std::cmp_equal(pdis->itemID, -1))
        {
            break;
        }

        switch (pdis->itemAction)
        {
        case ODA_SELECT:
        case ODA_DRAWENTIRE: {
            const auto item = reinterpret_cast<t_listbox_item *>(ListBox_GetItemData(g_ctx.listbox_hwnd, pdis->itemID));

            COLORREF text_color;
            HBRUSH bg_brush;

            if (pdis->itemState & ODS_SELECTED && item->selectable())
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
            if (item->activatable)
            {
                int32_t state_id;
                if (item->enabled)
                {
                    state_id = item->active ? CBS_CHECKEDNORMAL : CBS_UNCHECKEDNORMAL;
                }
                else
                {
                    state_id = item->active ? CBS_CHECKEDDISABLED : CBS_UNCHECKEDDISABLED;
                }

                SIZE checkbox_size{};
                GetThemePartSize(g_ctx.button_theme, nullptr, BP_CHECKBOX, state_id, nullptr, TS_TRUE, &checkbox_size);
                checkbox_width = checkbox_size.cx;

                RECT rc = pdis->rcItem;
                rc.left += 12;
                rc.right = rc.left + checkbox_width;
                rc.bottom = rc.top + checkbox_width;
                DrawThemeBackground(g_ctx.button_theme, pdis->hDC, BP_CHECKBOX, state_id, &rc, nullptr);
            }

            // 3. Draw the display text if applicable
            SetBkMode(pdis->hDC, TRANSPARENT);
            SetTextColor(pdis->hDC, text_color);

            RECT base_rc = pdis->rcItem;
            base_rc.left += 12;
            if (checkbox_width > 0)
            {
                base_rc.left += checkbox_width + 4;
            }

            if (!item->is_group)
            {
                const auto draw_flag = item->enabled ? 0 : DSS_DISABLED;

                DrawState(pdis->hDC, nullptr, nullptr, (LPARAM)item->text.c_str(), 0, base_rc.left, base_rc.top,
                          base_rc.right - base_rc.left, base_rc.bottom - base_rc.top, draw_flag | DST_TEXT);
            }

            // 4. Draw the hint text if applicable
            if (!item->hint_text.empty())
            {
                const auto hint_text = limit_wstring(item->hint_text, 30);

                const auto draw_flag = item->enabled ? 0 : DSS_DISABLED;

                SIZE sz;
                GetTextExtentPoint32(pdis->hDC, hint_text.c_str(), (int)hint_text.size(), &sz);
                const int x = base_rc.right - sz.cx;

                DrawState(pdis->hDC, nullptr, nullptr, (LPARAM)hint_text.c_str(), 0, x, base_rc.top, sz.cx,
                          base_rc.bottom - base_rc.top, draw_flag | DSS_RIGHT | DST_TEXT);
            }

            // 5. Draw the group header if applicable
            if (item->is_group)
            {
                const RECT text_rc = pdis->rcItem;

                DrawState(pdis->hDC, nullptr, nullptr, (LPARAM)item->text.c_str(), 0, text_rc.left, text_rc.top,
                          text_rc.right - text_rc.left, text_rc.bottom - text_rc.top, DST_TEXT);

                HPEN pen = CreatePen(PS_DOT, 1, text_color);
                HGDIOBJ prev_obj = SelectObject(pdis->hDC, pen);

                SIZE sz;
                GetTextExtentPoint32(pdis->hDC, item->text.c_str(), (int)item->text.length(), &sz);

                MoveToEx(pdis->hDC, text_rc.left + sz.cx + 4, text_rc.top + (text_rc.bottom - text_rc.top) / 2,
                         nullptr);
                LineTo(pdis->hDC, text_rc.right, text_rc.top + (text_rc.bottom - text_rc.top) / 2);

                SelectObject(pdis->hDC, prev_obj);
            }

            // 6. Draw the focus rect
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
    const HWND hwnd =
        CreateDialog(g_main_ctx.hinst, MAKEINTRESOURCE(IDD_COMMAND_PALETTE), g_main_ctx.hwnd, command_palette_proc);
    ShowWindow(hwnd, SW_SHOW);
}

HWND CommandPalette::hwnd()
{
    return g_ctx.hwnd;
}
