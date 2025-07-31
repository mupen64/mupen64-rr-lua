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
    bool has_separator{};

private:
    std::wstring m_raw_name{};

public:
    explicit t_menu_item(const std::wstring& name)
    {
        this->m_raw_name = name;
        this->has_separator = name.ends_with(SEPARATOR_SUFFIX);
    }

    [[nodiscard]] auto raw_name() const
    {
        return m_raw_name;
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


std::wstring t_menu_item::display_name() const
{
    auto display_name = has_separator ? m_raw_name.substr(0, m_raw_name.size() - SEPARATOR_SUFFIX.size()) : m_raw_name;

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
 * \brief Splits a fully-qualified action path into its components.
 */
static std::vector<std::wstring> split_action_path(const std::wstring& path)
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
 * \brief Performs a depth-first iteration over the command tree, applying the given predicate to each node. The predicate is also applied to the initial node itself.
 */
static void iterate_all_children_and_self(t_menu_item& node, const std::function<void(t_menu_item& node)>& predicate)
{
    predicate(node);
    for (auto& child : node.children)
    {
        iterate_all_children_and_self(child, predicate);
    }
}

/**
 * \brief Walks the command tree to find the command node corresponding to the "Name" segment of the fully-qualified action path.
 */
static t_menu_item* find_command_node_matching_path_name(const std::wstring& path)
{
    const auto segments = split_action_path(path);

    if (segments.empty())
    {
        return nullptr;
    }

    const auto& last_segment = segments.back();

    t_menu_item* found_node = nullptr;
    iterate_all_children_and_self(g_mgr.menu, [&](t_menu_item& node) {
        if (node.raw_name() == last_segment)
        {
            found_node = &node;
        }
    });

    return found_node;
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
    iterate_all_children_and_self(g_mgr.menu, [&](const t_menu_item& node) {
        if (!node.action)
        {
            return;
        }
        const bool enabled = node.action->params.get_enabled();
        EnableMenuItem(main_menu, node.id, enabled ? MF_ENABLED : MF_GRAYED);
    });
}

/**
 * \brief Updates the active states of all menu items.
 */
static void update_menu_active_states()
{
    const HMENU main_menu = GetMenu(g_main_hwnd);
    iterate_all_children_and_self(g_mgr.menu, [&](const t_menu_item& node) {
        if (!node.action)
        {
            return;
        }
        const bool checked = node.action->params.get_active();
        CheckMenuItem(main_menu, node.id, checked ? MF_CHECKED : MF_UNCHECKED);
    });
}

/**
 * \brief Updates the names of all menu items.
 */
static void update_menu_names()
{
    iterate_all_children_and_self(g_mgr.menu, [&](const t_menu_item& node) {
        if (!node.has_menu)
        {
            return;
        }

        auto display_name = node.display_name();

        // Add the accelerator text if there is any :P
        if (node.action && g_config.hotkeys.contains(node.action->params.path))
        {
            const auto hotkey = g_config.hotkeys[node.action->params.path];
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

        if (node.children.empty())
        {
            if (!SetMenuItemInfo(node.parent_menu, node.id, false, &mii))
            {
                g_view_logger->error(L"ActionManager::update_menu_names: Couldn't update name of '{}'.", display_name);
            }
        }

        if (!SetMenuItemInfo(node.parent_menu, node.position_under_parent, TRUE, &mii))
        {
            g_view_logger->error(L"ActionManager::update_menu_names: Couldn't update name of popup '{}'.", display_name);
        }
    });
}

bool ActionManager::add(const t_action_params& params)
{
    t_action action{};
    action.params = params;

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
    if (!validate_action_path(path))
    {
        g_view_logger->error(L"ActionManager::associate_hotkey: Malformed action path '{}'.", path);
        return false;
    }

    t_action* action = find_action_by_path(path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::associate_hotkey: Action '{}' not found.", path);
        return false;
    }

    if (!g_config.hotkeys.contains(path))
    {
        g_view_logger->debug(L"ActionManager::associate_hotkey: Initial hotkey registered for '{}': {}.", path, hotkey.to_wstring());
        g_config.inital_hotkeys[path] = hotkey;
    }

    g_view_logger->debug(L"ActionManager::associate_hotkey: Hotkey registered for '{}': {}.", path, hotkey.to_wstring());
    g_config.hotkeys[path] = hotkey;

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
    if (!validate_action_path(path))
    {
        g_view_logger->error(L"ActionManager::notify_enabled_changed: Malformed action path '{}'.", path);
        return;
    }

    t_action* action = find_action_by_path(path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::notify_enabled_changed: Action '{}' not found.", path);
        return;
    }

    g_view_logger->debug(L"ActionManager::notify_enabled_changed: Action '{}' enabled changed.", path);

    // TODO: Implement this properly by the spec

    update_menu_enabled_states();
}

void ActionManager::notify_active_changed(const std::wstring& path)
{
    if (!validate_action_path(path))
    {
        g_view_logger->error(L"ActionManager::notify_active_changed: Malformed action path '{}'.", path);
        return;
    }

    t_action* action = find_action_by_path(path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::notify_active_changed: Action '{}' not found.", path);
        return;
    }

    g_view_logger->debug(L"ActionManager::notify_active_changed: Action '{}' checked changed.", path);

    // TODO: Implement this properly by the spec
    update_menu_active_states();
}

void ActionManager::notify_real_name_changed(const std::wstring& path)
{
    // TODO: Implement this properly by the spec
    update_menu_names();
}

std::wstring ActionManager::get_action_friendly_name(const std::wstring& path)
{
    const auto node = find_command_node_matching_path_name(path);

    if (!node)
    {
        g_view_logger->error(L"ActionManager::get_action_friendly_name: Action '{}' has no node.", path);
        return L"";
    }

    return node->display_name();
}

bool ActionManager::handle_menu_interaction(size_t id)
{
    t_action* action = nullptr;
    iterate_all_children_and_self(g_mgr.menu, [&](const t_menu_item& node) {
        if (node.id == id)
        {
            action = node.action;
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
    if (!validate_action_path(path))
    {
        g_view_logger->error(L"ActionManager::invoke: Malformed action path '{}'.", path);
        return;
    }

    t_action* action = find_action_by_path(path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::invoke: Action with path '{}' not found.", path);
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
        std::vector<std::wstring> parts = split_action_path(action.params.path);

        t_menu_item* current = &g_mgr.menu;

        for (const auto& part : parts)
        {
            auto it = std::ranges::find_if(current->children,
                                           [&](const t_menu_item& node) {
                                               return node.raw_name() == part;
                                           });

            if (it != current->children.end())
            {
                current = &*it;
            }
            else
            {
                current->children.emplace_back(part);
                current = &current->children.back();
            }
        }
    }

    for (auto& action : g_mgr.actions)
    {
        const auto command = find_command_node_matching_path_name(action.params.path);
        if (!command)
        {
            g_view_logger->error(L"Failed to find command node for action: {}", action.params.path);
            continue;
        }
        command->action = &action;
    }
}

/**
 * \brief Logs the structure of the command tree to the logger.
 */
static void log_menu_structure(const t_menu_item& node, size_t depth = 0)
{
    if (depth == 0)
    {
        g_view_logger->debug(L"---- Menu structure ----");
    }

    std::wstring indent(depth * 2, L' ');
    g_view_logger->debug(L"{} {}", indent, node.raw_name());

    for (const auto& child : node.children)
    {
        log_menu_structure(child, depth + 1);
    }

    if (depth == 0)
    {
        g_view_logger->debug(L"---- End of menu structure ----");
    }
}

/**
 * \brief Adds menu items to the specified parent menu based on the command tree structure.
 */
static void add_menu_items(t_menu_item& node, const HMENU parent_menu, const size_t depth = 0)
{
    g_mgr.menu_id_counter++;
    runtime_assert(g_mgr.menu_id_counter <= IDM_RESERVED_END, std::format(L"Menu ID counter overflow: {} (max {})", g_mgr.menu_id_counter, IDM_RESERVED_END));

    node.id = (uint16_t)g_mgr.menu_id_counter;
    node.parent_menu = parent_menu;
    node.position_under_parent = GetMenuItemCount(parent_menu);
    node.has_menu = true;

    if (node.children.empty())
    {
        AppendMenu(parent_menu, MF_STRING, node.id, node.display_name().c_str());

        if (node.has_separator)
        {
            AppendMenu(parent_menu, MF_SEPARATOR, 0, nullptr);
        }
        return;
    }

    node.popup_handle = CreatePopupMenu();
    AppendMenu(parent_menu, MF_STRING | MF_POPUP, (UINT_PTR)node.popup_handle, node.display_name().c_str());

    if (node.has_separator)
    {
        AppendMenu(parent_menu, MF_SEPARATOR, 0, nullptr);
    }

    for (auto& child_node : node.children)
    {
        add_menu_items(child_node, node.popup_handle, depth + 1);
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
    for (auto& node : g_mgr.menu.children.at(0).children)
    {
        add_menu_items(node, main_menu_bar);
    }
    for (size_t i = 1; i < g_mgr.menu.children.size(); ++i)
    {
        for (auto& node : g_mgr.menu.children[i].children)
        {
            add_menu_items(node, main_menu_bar);
        }
    }


    // 5. Update all the stuff relevant to the menu.
    update_menu_enabled_states();
    update_menu_active_states();
    update_menu_names();
}
