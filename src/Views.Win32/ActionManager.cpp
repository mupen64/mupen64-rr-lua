/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ActionManager.h>

using t_action_params = ActionManager::t_action_params;

const std::wstring SEPARATOR_SUFFIX = L" ---";

struct t_action {
    t_action_params params{};
};

struct t_menu_item {
    uint16_t id{};
    size_t position_under_parent{};
    HMENU popup_handle{};
    HMENU parent_menu{};
    bool has_menu{};

    t_action* action{};
    std::vector<t_menu_item> children{};

private:
    std::wstring m_path{};
    std::wstring m_name{};
    bool m_has_separator{};

public:
    explicit t_menu_item(const std::wstring& path);

    [[nodiscard]] auto raw_path() const
    {
        return m_path;
    }

    [[nodiscard]] bool has_separator() const
    {
        return m_has_separator;
    }

    [[nodiscard]] std::wstring display_name() const;
};

struct t_action_manager {
    std::vector<t_action> actions{};
    t_menu_item menu{L"Root"};
    size_t menu_id_counter{};
    bool batched_work{};
};

static t_action_manager g_mgr{};

static void build_menu();

/**
 * \brief Splits a fully-qualified action path into its components.
 */
static std::vector<std::wstring> split_path(const std::wstring& path)
{
    std::vector<std::wstring> parts = io_service.split_wstring(path, L">");
    for (auto& part : parts)
    {
        part = io_service.trim(part);
    }

    std::erase_if(parts, [](const std::wstring& part) {
        return part.empty();
    });

    return parts;
}

/**
 * \brief Normalizes an action's path by deconstructing it into segments, then reconstructing it with a consistent format.
 */
static std::wstring normalize_path(const std::wstring& path)
{
    const auto parts = split_path(path);
    return io_service.join_wstring(parts, L">");
}

t_menu_item::t_menu_item(const std::wstring& path)
{
    this->m_path = path;
    this->m_name = split_path(path).back();
    this->m_has_separator = this->m_name.ends_with(SEPARATOR_SUFFIX);

    if (this->m_has_separator)
    {
        this->m_name = m_name.substr(0, m_name.size() - SEPARATOR_SUFFIX.size());
    }
}

std::wstring t_menu_item::display_name() const
{
    auto display_name = m_name;

    if (action && action->params.get_real_name)
    {
        const auto real_name = action->params.get_real_name();
        if (!real_name.empty())
        {
            display_name = real_name;
        }
    }

    return display_name;
}

/**
 * \brief Performs a depth-first iteration over the menu item tree, applying the given predicate to each item. The predicate is also applied to the initial item itself.
 */
static void iterate_all_children_and_self(t_menu_item& item, const std::function<void(t_menu_item& item)>& predicate)
{
    predicate(item);
    for (auto& child : item.children)
    {
        iterate_all_children_and_self(child, predicate);
    }
}

/**
 * \brief Walks the command tree to find the command item corresponding to the "Name" segment of the fully-qualified action path.
 */
static t_menu_item* find_item_by_path(const std::wstring& path)
{
    t_menu_item* found_item = nullptr;
    iterate_all_children_and_self(g_mgr.menu, [&](t_menu_item& item) {
        if (item.raw_path() == path)
        {
            found_item = &item;
        }
    });

    return found_item;
}

/**
 * \brief Tries to find an action by its fully-qualified path.
 */
static t_action* find_action_by_path(const std::wstring& path)
{
    for (auto& a : g_mgr.actions)
    {
        if (a.params.path == path)
        {
            return &a;
        }
    }
    return nullptr;
}

/**
 * \brief Checks whether the given fully-qualified action path is valid.
 */
static bool validate_action_path(const std::wstring& path)
{
    if (path.empty())
    {
        g_view_logger->error(L"Action path cannot be empty.");
        return false;
    }

    if (path.find(L'>') == std::wstring::npos)
    {
        g_view_logger->error(L"Action path must contain at least one '>'.");
        return false;
    }

    return true;
}

/**
 * \brief Updates the enabled states of all menu items.
 */
static void update_menu_enabled_states()
{
    const HMENU main_menu = GetMenu(g_main_hwnd);
    iterate_all_children_and_self(g_mgr.menu, [&](const t_menu_item& item) {
        if (!item.action)
        {
            return;
        }
        const bool enabled = item.action->params.get_enabled();
        EnableMenuItem(main_menu, item.id, enabled ? MF_ENABLED : MF_GRAYED);
    });
}

/**
 * \brief Updates the active states of all menu items.
 */
static void update_menu_active_states()
{
    const HMENU main_menu = GetMenu(g_main_hwnd);
    iterate_all_children_and_self(g_mgr.menu, [&](const t_menu_item& item) {
        if (!item.action)
        {
            return;
        }
        const bool checked = item.action->params.get_active();
        CheckMenuItem(main_menu, item.id, checked ? MF_CHECKED : MF_UNCHECKED);
    });
}

/**
 * \brief Updates the names of all menu items.
 */
static void update_menu_names()
{
    iterate_all_children_and_self(g_mgr.menu, [&](const t_menu_item& item) {
        if (!item.has_menu)
        {
            return;
        }

        auto display_name = item.display_name();

        // Add the accelerator text if there is any :P
        if (item.action && g_config.hotkeys.contains(item.action->params.path))
        {
            const auto hotkey = g_config.hotkeys[item.action->params.path];
            if (!hotkey.is_nothing())
            {
                display_name += std::format(L"\t{}", hotkey.to_wstring());
            }
        }

        MENUITEMINFO mii{};
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_STRING;
        mii.dwTypeData = const_cast<LPWSTR>(display_name.c_str());
        mii.cch = display_name.length();

        if (item.children.empty())
        {
            if (!SetMenuItemInfo(item.parent_menu, item.id, false, &mii))
            {
                g_view_logger->error(L"ActionManager::update_menu_names: Couldn't update name of '{}'.", display_name);
            }
        }

        if (!SetMenuItemInfo(item.parent_menu, item.position_under_parent, TRUE, &mii))
        {
            g_view_logger->error(L"ActionManager::update_menu_names: Couldn't update name of popup '{}'.", display_name);
        }
    });
}

bool ActionManager::add(const t_action_params& params)
{
    t_action action{};
    action.params = params;
    action.params.path = normalize_path(action.params.path);

    if (!validate_action_path(action.params.path))
    {
        g_view_logger->error(L"ActionManager::add: Malformed action path '{}'.", params.path);
        return false;
    }

    std::erase_if(g_mgr.actions, [&](const t_action& a) {
        return a.params.path == action.params.path;
    });

    g_mgr.actions.emplace_back(action);

    if (!g_mgr.batched_work)
    {
        build_menu();
    }

    return true;
}

bool ActionManager::associate_hotkey(const std::wstring& path, const Hotkey::t_hotkey& hotkey)
{
    const auto normalized_path = normalize_path(path);

    if (!validate_action_path(normalized_path))
    {
        g_view_logger->error(L"ActionManager::associate_hotkey: Malformed action path '{}'.", normalized_path);
        return false;
    }

    t_action* action = find_action_by_path(normalized_path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::associate_hotkey: Action '{}' not found.", normalized_path);
        return false;
    }

    if (!g_config.hotkeys.contains(normalized_path))
    {
        g_view_logger->debug(L"ActionManager::associate_hotkey: Initial hotkey registered for '{}': {}.", normalized_path, hotkey.to_wstring());
        g_config.inital_hotkeys[normalized_path] = hotkey;
    }

    g_view_logger->debug(L"ActionManager::associate_hotkey: Hotkey registered for '{}': {}.", normalized_path, hotkey.to_wstring());
    g_config.hotkeys[normalized_path] = hotkey;

    if (!g_mgr.batched_work)
    {
        build_menu();
    }

    return true;
}

void ActionManager::begin_batch_work()
{
    g_mgr.batched_work = true;
}

void ActionManager::end_batch_work()
{
    g_mgr.batched_work = false;
    build_menu();
}

void ActionManager::notify_enabled_changed(const std::wstring& path)
{
    const auto normalized_path = normalize_path(path);

    if (!validate_action_path(normalized_path))
    {
        g_view_logger->error(L"ActionManager::notify_enabled_changed: Malformed action path '{}'.", normalized_path);
        return;
    }

    t_action* action = find_action_by_path(normalized_path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::notify_enabled_changed: Action '{}' not found.", normalized_path);
        return;
    }

    g_view_logger->debug(L"ActionManager::notify_enabled_changed: Action '{}' enabled changed.", normalized_path);

    // TODO: Implement this properly by the spec

    update_menu_enabled_states();
}

void ActionManager::notify_active_changed(const std::wstring& path)
{
    const auto normalized_path = normalize_path(path);

    if (!validate_action_path(normalized_path))
    {
        g_view_logger->error(L"ActionManager::notify_active_changed: Malformed action path '{}'.", normalized_path);
        return;
    }

    t_action* action = find_action_by_path(normalized_path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::notify_active_changed: Action '{}' not found.", normalized_path);
        return;
    }

    g_view_logger->debug(L"ActionManager::notify_active_changed: Action '{}' checked changed.", normalized_path);

    // TODO: Implement this properly by the spec
    update_menu_active_states();
}

void ActionManager::notify_real_name_changed(const std::wstring&)
{
    // TODO: Implement this properly by the spec
    update_menu_names();
}

std::wstring ActionManager::get_action_friendly_name(const std::wstring& path)
{
    const auto normalized_path = normalize_path(path);

    const auto item = find_item_by_path(normalized_path);

    if (!item)
    {
        g_view_logger->error(L"ActionManager::get_action_friendly_name: Action '{}' has no node.", normalized_path);
        return L"";
    }

    return item->display_name();
}

bool ActionManager::handle_menu_interaction(size_t id)
{
    t_action* action = nullptr;
    iterate_all_children_and_self(g_mgr.menu, [&](const t_menu_item& item) {
        if (item.id == id)
        {
            action = item.action;
        }
    });

    if (!action)
    {
        return false;
    }

    g_view_logger->debug(L"ActionManager::handle_menu_interaction: Invoking '{}' (#{}).", action->params.path, id);
    action->params.down_callback();

    return true;
}

void ActionManager::invoke(const std::wstring& path)
{
    const auto normalized_path = normalize_path(path);

    if (!validate_action_path(normalized_path))
    {
        g_view_logger->error(L"ActionManager::invoke: Malformed action path '{}'.", normalized_path);
        return;
    }

    t_action* action = find_action_by_path(normalized_path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::invoke: Action with path '{}' not found.", normalized_path);
        return;
    }

    action->params.down_callback();
}

/**
 * \brief Builds the initial menu tree based on the registered actions' paths.
 */
static void build_initial_menu_tree()
{
    g_mgr.menu = t_menu_item(L"Root");

    for (const auto& action : g_mgr.actions)
    {
        std::vector<std::wstring> parts = split_path(action.params.path);

        t_menu_item* current = &g_mgr.menu;

        for (int i = 0; i < parts.size(); ++i)
        {
            std::wstring path_up_to_here = io_service.join_wstring(std::vector(parts.begin(), parts.begin() + i + 1), L">");

            auto it = std::ranges::find_if(current->children,
                                           [&](const t_menu_item& item) {
                                               return item.raw_path() == path_up_to_here;
                                           });

            if (it != current->children.end())
            {
                current = &*it;
            }
            else
            {
                current->children.emplace_back(path_up_to_here);
                current = &current->children.back();
            }
        }
    }

    for (auto& action : g_mgr.actions)
    {
        const auto item = find_item_by_path(action.params.path);
        runtime_assert(item, std::format(L"ActionManager::build_initial_menu_tree: Action '{}' has no node.", action.params.path));
        item->action = &action;
    }
}

/**
 * \brief Adds menu items to the specified parent menu based on the command tree structure.
 */
static void add_menu_items(t_menu_item& item, const HMENU parent_menu)
{
    g_mgr.menu_id_counter++;
    runtime_assert(g_mgr.menu_id_counter <= IDM_RESERVED_END, std::format(L"Menu ID counter overflow: {} (max {})", g_mgr.menu_id_counter, IDM_RESERVED_END));

    item.id = (uint16_t)g_mgr.menu_id_counter;
    item.parent_menu = parent_menu;
    item.position_under_parent = GetMenuItemCount(parent_menu);
    item.has_menu = true;

    if (item.children.empty())
    {
        AppendMenu(parent_menu, MF_STRING, item.id, item.display_name().c_str());

        if (item.has_separator())
        {
            AppendMenu(parent_menu, MF_SEPARATOR, 0, nullptr);
        }
        return;
    }

    item.popup_handle = CreatePopupMenu();
    AppendMenu(parent_menu, MF_STRING | MF_POPUP, (UINT_PTR)item.popup_handle, item.display_name().c_str());

    if (item.has_separator())
    {
        AppendMenu(parent_menu, MF_SEPARATOR, 0, nullptr);
    }

    for (auto& child : item.children)
    {
        add_menu_items(child, item.popup_handle);
    }
}

static void build_menu()
{
    build_initial_menu_tree();

    // 1. Delete all existing menu items
    const HMENU main_menu_bar = GetMenu(g_main_hwnd);
    const auto menu_count = GetMenuItemCount(main_menu_bar);
    for (int i = 0; i < menu_count; ++i)
    {
        DeleteMenu(main_menu_bar, 0, MF_BYPOSITION);
    }

    // 2. Add the built-in commands (flat) followed by the other ones.
    g_mgr.menu_id_counter = 0;
    for (auto& item : g_mgr.menu.children.at(0).children)
    {
        add_menu_items(item, main_menu_bar);
    }
    for (size_t i = 1; i < g_mgr.menu.children.size(); ++i)
    {
        for (auto& item : g_mgr.menu.children[i].children)
        {
            add_menu_items(item, main_menu_bar);
        }
    }

    // 5. Update all the stuff relevant to the menu.
    // update_menu_enabled_states();
    // update_menu_active_states();
    // update_menu_names();
}
