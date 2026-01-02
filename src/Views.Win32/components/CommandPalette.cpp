/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "RomBrowser.h"
#include "stdafx.h"
#include <components/CommandPalette.h>
#include <components/ConfigDialog.h>
#include <components/AppActions.h>

struct t_listbox_item
{
    struct t_group_data
    {
        std::wstring text;
    };

    struct t_action_data
    {
        std::wstring text;
        std::wstring path{};
        std::wstring raw_display_name{};
        std::wstring hotkey{};

        bool enabled{};
        bool active{};
        bool activatable{};
    };

    struct t_option_data
    {
        ConfigDialog::t_options_item *item{};
    };

    struct t_rom_data
    {
        RomBrowser::t_simple_rom_info rom{};
    };

    std::variant<t_group_data, t_action_data, t_option_data, t_rom_data> data{};

    static t_listbox_item make_group(const std::wstring &group_name);
    static t_listbox_item make_action(const std::wstring &action, const std::wstring &group);
    static t_listbox_item make_option(ConfigDialog::t_options_item *item, const ConfigDialog::t_options_group &group);
    static t_listbox_item make_option_group(const ConfigDialog::t_options_group &options_group);
    static t_listbox_item make_rom(const RomBrowser::t_simple_rom_info &rom);

    /**
     * \return Whether the item is selectable.
     */
    [[nodiscard]] bool selectable() const;

    /**
     * \return Whether the item is enabled.
     */
    [[nodiscard]] bool enabled() const;

    /**
     * \return Whether the item matches the specified search query.
     */
    [[nodiscard]] bool matches_query(const std::wstring_view query) const;

    /**
     * \return The primary text to display for this item, if any.
     */
    [[nodiscard]] std::optional<std::wstring> get_primary_text() const;

    /**
     * \return The secondary text to display for this item, if any.
     */
    [[nodiscard]] std::optional<std::wstring> get_secondary_text() const;

  private:
    t_listbox_item() = default;
};

struct t_command_palette_context
{
    HWND hwnd{};
    HWND text_hwnd{};
    HWND listbox_hwnd{};
    HWND edit_hwnd{};

    HTHEME button_theme{};

    bool closing{};
    bool dont_close_on_focus_loss{};

    // All items, built once when the command palette is shown.
    std::vector<t_listbox_item> all_items{};
    // Currently displayed items, filtered by search query.
    std::vector<t_listbox_item> items{};

    std::wstring search_query{};
    std::vector<std::wstring> actions{};
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
    item.data = t_group_data{group_name};
    return item;
}

t_listbox_item t_listbox_item::make_action(const std::wstring &action, const std::wstring &group)
{
    const auto hotkey = g_config.hotkeys.at(action);
    const auto hotkey_str = hotkey.is_empty() ? L"" : hotkey.to_wstring();

    t_listbox_item item{};
    item.data = t_action_data{.text = ActionManager::get_display_name(action),
                              .path = action,
                              .raw_display_name = ActionManager::get_display_name(action, true),
                              .hotkey = hotkey_str,
                              .enabled = ActionManager::get_enabled(action),
                              .active = ActionManager::get_active(action),
                              .activatable = ActionManager::get_activatability(action)};
    return item;
}

t_listbox_item t_listbox_item::make_option(ConfigDialog::t_options_item *options_item,
                                           const ConfigDialog::t_options_group &group)
{
    t_listbox_item item{};
    item.data = t_option_data{.item = options_item};
    return item;
}

t_listbox_item t_listbox_item::make_option_group(const ConfigDialog::t_options_group &options_group)
{
    t_listbox_item item{};
    item.data = t_group_data{.text = options_group.name};
    return item;
}

t_listbox_item t_listbox_item::make_rom(const RomBrowser::t_simple_rom_info &rom)
{
    t_listbox_item item{};
    item.data = t_rom_data{.rom = rom};
    return item;
}

bool t_listbox_item::selectable() const
{
    if (!this->enabled()) return false;

    if (std::holds_alternative<t_group_data>(data))
    {
        return false;
    }

    if (std::holds_alternative<t_action_data>(data))
    {
        return std::get<t_action_data>(data).enabled;
    }

    if (std::holds_alternative<t_option_data>(data))
    {
        return std::get<t_option_data>(data).item->is_readonly() == false;
    }

    return true;
}

bool t_listbox_item::matches_query(const std::wstring_view query) const
{
    if (query.empty())
    {
        return true;
    }

    if (std::holds_alternative<t_group_data>(data))
    {
        const auto &text = std::get<t_group_data>(data).text;
        const auto normalized_text = normalize(text);
        return normalized_text.contains(query);
    }

    if (std::holds_alternative<t_rom_data>(data))
    {
        const auto &rom = std::get<t_rom_data>(data).rom;
        const auto filename = std::filesystem::path(rom.path).filename().wstring();
        const auto normalized_filename = normalize(filename);
        return normalized_filename.contains(query);
    }

    if (std::holds_alternative<t_action_data>(data))
    {
        const auto &action = std::get<t_action_data>(data);
        const auto normalized_text = normalize(action.text);
        const auto normalized_path = normalize(action.path);
        const auto normalized_display_name = normalize(action.raw_display_name);

        return normalized_display_name.contains(query) || normalized_text.contains(query) ||
               normalized_path.contains(query);
    }

    if (std::holds_alternative<t_option_data>(data))
    {
        const auto &item = std::get<t_option_data>(data).item;
        const auto display_name = item->get_name();
        const auto normalized_display_name = normalize(display_name);
        return normalized_display_name.contains(query);
    }

    RT_ASSERT(false, L"Unknown listbox item type in matches_query");

    return false;
}

std::optional<std::wstring> t_listbox_item::get_primary_text() const
{
    if (std::holds_alternative<t_group_data>(data))
    {
        return std::get<t_group_data>(data).text;
    }

    if (std::holds_alternative<t_rom_data>(data))
    {
        const auto &rom = std::get<t_rom_data>(data).rom;
        return std::filesystem::path(rom.path).filename().wstring();
    }

    if (std::holds_alternative<t_action_data>(data))
    {
        const auto &action = std::get<t_action_data>(data);
        return action.text;
    }

    if (std::holds_alternative<t_option_data>(data))
    {
        const auto &item = std::get<t_option_data>(data).item;
        return item->get_name();
    }

    return std::nullopt;
}

std::optional<std::wstring> t_listbox_item::get_secondary_text() const
{
    if (std::holds_alternative<t_action_data>(data))
    {
        const auto &action = std::get<t_action_data>(data);
        return action.hotkey;
    }

    if (std::holds_alternative<t_option_data>(data))
    {
        const auto &item = std::get<t_option_data>(data).item;
        return item->get_value_name();
    }

    return std::nullopt;
}

bool t_listbox_item::enabled() const
{
    if (std::holds_alternative<t_action_data>(data))
    {
        return std::get<t_action_data>(data).enabled;
    }

    if (std::holds_alternative<t_option_data>(data))
    {
        return std::get<t_option_data>(data).item->is_readonly() == false;
    }

    return true;
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

    const auto item = reinterpret_cast<t_listbox_item *>(ListBox_GetItemData(g_ctx.listbox_hwnd, i));

    if (std::holds_alternative<t_listbox_item::t_action_data>(item->data))
    {
        const auto &action = std::get<t_listbox_item::t_action_data>(item->data);
        SendMessage(g_ctx.hwnd, WM_CLOSE, 0, 0);
        ActionManager::invoke(action.path);
        return true;
    }

    if (std::holds_alternative<t_listbox_item::t_option_data>(item->data))
    {
        const auto &option = std::get<t_listbox_item::t_option_data>(item->data);

        // HACK: We want to keep the command palette open (in case the user cancels and wants to keep looking through
        // the command palette) while editing the option, but we also want to prevent it from closing
        EnableWindow(g_ctx.hwnd, false);
        g_ctx.dont_close_on_focus_loss = true;
        const auto confirmed = option.item->edit(g_ctx.hwnd);
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

    if (std::holds_alternative<t_listbox_item::t_rom_data>(item->data))
    {
        const auto &rom = std::get<t_listbox_item::t_rom_data>(item->data);
        SendMessage(g_ctx.hwnd, WM_CLOSE, 0, 0);
        AppActions::load_rom_from_path(rom.rom.path);
        return true;
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

    if (std::holds_alternative<t_listbox_item::t_action_data>(item->data))
    {
        const auto &action = std::get<t_listbox_item::t_action_data>(item->data);

        Hotkey::t_hotkey hotkey = g_config.hotkeys.at(action.path);
        Hotkey::show_prompt(g_main_ctx.hwnd, std::format(L"Choose a hotkey for {}", action.text), hotkey);
        Hotkey::try_associate_hotkey(g_main_ctx.hwnd, action.path, hotkey);
        return true;
    }

    return false;
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

            return false;
        });

        if (actions.empty())
        {
            continue;
        }

        g_ctx.all_items.emplace_back(t_listbox_item::make_group(name));

        for (const auto &action : actions)
        {
            g_ctx.all_items.emplace_back(t_listbox_item::make_action(action, group));
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
            return item.type == ConfigDialog::t_options_item::Type::Hotkey;
        });

        if (group.items.empty())
        {
            continue;
        }

        g_ctx.all_items.emplace_back(t_listbox_item::make_option_group(group));

        for (auto &item : group.items)
        {
            g_ctx.all_items.emplace_back(t_listbox_item::make_option(&item, group));
        }
    }
}

/**
 * \brief Adds known ROMs to the listbox item collection.
 */
static void add_roms(const std::wstring_view query)
{
    g_ctx.all_items.emplace_back(t_listbox_item::make_group(L"ROMs"));

    for (const auto &rom : RomBrowser::get_discovered_roms())
    {
        g_ctx.all_items.emplace_back(t_listbox_item::make_rom(rom));
    }
}

/**
 * \brief Collects all listbox items.
 */
static void collect_items()
{
    g_ctx.all_items = {};
    const auto normalized_query = normalize(g_ctx.search_query);

    add_actions(normalized_query);
    add_options(normalized_query);
    add_roms(normalized_query);
}

/**
 * \brief Refreshes the listbox contents based on the current search query.
 */
static void refresh_listbox()
{
    g_ctx.items = {};

    const auto normalized_query = normalize(g_ctx.search_query);
    for (size_t i = 0; i < g_ctx.all_items.size(); i++)
    {
        const auto &item = g_ctx.all_items[i];

        if (!std::holds_alternative<t_listbox_item::t_group_data>(item.data)) continue;

        std::vector<t_listbox_item> matching_children;
        for (size_t j = i + 1; j < g_ctx.all_items.size(); j++)
        {
            const auto &child_item = g_ctx.all_items[j];
            if (std::holds_alternative<t_listbox_item::t_group_data>(child_item.data)) break;
            if (child_item.matches_query(normalized_query)) matching_children.emplace_back(child_item);
        }

        if (!matching_children.empty())
        {
            g_ctx.items.emplace_back(item);
            for (const auto &child_item : matching_children)
            {
                g_ctx.items.emplace_back(child_item);
            }
        }
    }

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
            const auto success = try_change_hotkey(selected_index);
            EnableWindow(g_ctx.hwnd, true);
            g_ctx.dont_close_on_focus_loss = false;

            if (!success) return FALSE;

            SetFocus(g_ctx.edit_hwnd);
            collect_items();
            refresh_listbox();

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
        collect_items();
        refresh_listbox();

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
                    refresh_listbox();
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

            const auto enabled = item->enabled();

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
            const auto has_checkbox = std::holds_alternative<t_listbox_item::t_action_data>(item->data) &&
                                      std::get<t_listbox_item::t_action_data>(item->data).activatable;
            int checkbox_width = 0;
            if (has_checkbox)
            {
                const auto &action = std::get<t_listbox_item::t_action_data>(item->data);
                int32_t state_id;
                if (enabled)
                {
                    state_id = action.active ? CBS_CHECKEDNORMAL : CBS_UNCHECKEDNORMAL;
                }
                else
                {
                    state_id = action.active ? CBS_CHECKEDDISABLED : CBS_UNCHECKEDDISABLED;
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

            // 3. Draw the primary text if applicable
            SetBkMode(pdis->hDC, TRANSPARENT);
            SetTextColor(pdis->hDC, text_color);

            RECT base_rc = pdis->rcItem;
            base_rc.left += 12;
            if (checkbox_width > 0)
            {
                base_rc.left += checkbox_width + 4;
            }
            if (std::holds_alternative<t_listbox_item::t_group_data>(item->data))
            {
                base_rc.left = 4;
            }

            const auto primary_text = item->get_primary_text();
            if (primary_text.has_value())
            {
                const auto draw_flag = enabled ? 0 : DSS_DISABLED;

                DrawState(pdis->hDC, nullptr, nullptr, (LPARAM)primary_text->c_str(), 0, base_rc.left, base_rc.top,
                          base_rc.right - base_rc.left, base_rc.bottom - base_rc.top, draw_flag | DST_TEXT);
            }

            // 4. Draw the secondary text if applicable
            const auto secondary_text = item->get_secondary_text();
            if (secondary_text.has_value())
            {
                const auto text = limit_wstring(*secondary_text, 30);
                const auto draw_flag = enabled ? 0 : DSS_DISABLED;

                SIZE sz;
                GetTextExtentPoint32(pdis->hDC, text.c_str(), (int)text.size(), &sz);
                const int x = base_rc.right - sz.cx;

                DrawState(pdis->hDC, nullptr, nullptr, (LPARAM)text.c_str(), 0, x, base_rc.top, sz.cx,
                          base_rc.bottom - base_rc.top, draw_flag | DSS_RIGHT | DST_TEXT);
            }

            if (std::holds_alternative<t_listbox_item::t_group_data>(item->data))
            {
                const auto group = std::get<t_listbox_item::t_group_data>(item->data);
                HPEN pen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
                HGDIOBJ prev_obj = SelectObject(pdis->hDC, pen);

                SIZE sz;
                GetTextExtentPoint32(pdis->hDC, group.text.c_str(), (int)group.text.length(), &sz);

                MoveToEx(pdis->hDC, base_rc.left + sz.cx + 4,
                         pdis->rcItem.top + (pdis->rcItem.bottom - pdis->rcItem.top) / 2, nullptr);
                LineTo(pdis->hDC, pdis->rcItem.right, pdis->rcItem.top + (pdis->rcItem.bottom - pdis->rcItem.top) / 2);

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
