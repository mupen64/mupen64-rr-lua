/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ActionManager.h>

using t_action_params = ActionManager::t_action_params;

struct t_action {
    t_action_params params{};
};

/**
 * \brief Represents a command associated with an action as part of a tree structure.
 */
struct t_command_node {
    /**
     * \brief The name of the node, which corresponds to a segment of the fully-qualified action path.
     */
    std::wstring name{};
    std::wstring display_name{};
    uint16_t menu_id{};
    t_action* action{};
    std::vector<t_command_node> children{};
    bool has_separator{};

    explicit t_command_node(const std::wstring& name)
    {
        const std::wstring separator_suffix = L" ---";

        this->name = name;
        this->display_name = name;

        if (name.ends_with(separator_suffix))
        {
            this->has_separator = true;
            this->display_name = name.substr(0, name.size() - separator_suffix.size());
        }
    }
};

struct t_action_manager {
    std::vector<t_action> actions{};
    t_command_node command_tree{L"root"};
};

static t_action_manager g_mgr{};

static void build_menu();

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
static void iterate_all_children_and_self(t_command_node& node, const std::function<void(t_command_node& node)>& predicate)
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
static t_command_node* find_command_node_matching_path_name(const std::wstring& path)
{
    const auto segments = split_action_path(path);

    if (segments.empty())
    {
        return nullptr;
    }

    const auto& last_segment = segments.back();

    t_command_node* found_node = nullptr;
    iterate_all_children_and_self(g_mgr.command_tree, [&](t_command_node& node) {
        if (node.name == last_segment)
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

    build_menu();

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
        g_view_logger->info(L"ActionManager::associate_hotkey: Initial hotkey registered for '{}': {}.", path, hotkey.to_wstring());
        g_config.inital_hotkeys[path] = hotkey;
    }

    g_view_logger->info(L"ActionManager::associate_hotkey: Hotkey registered for '{}': {}.", path, hotkey.to_wstring());
    g_config.hotkeys[path] = hotkey;

    build_menu();

    return true;
}

bool ActionManager::handle_menu_interaction(size_t id)
{
    t_action* action = nullptr;
    iterate_all_children_and_self(g_mgr.command_tree, [&](const t_command_node& node) {
        if (node.menu_id == id)
        {
            action = node.action;
        }
    });

    if (!action)
    {
        return false;
    }

    g_view_logger->info(L"ActionManager::handle_menu_interaction: Invoking '{}' (#{}).", action->params.path, id);
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
 * \brief Builds a tree structure containing command nodes based on the currently registered actions' paths.
 * The tree structure allows for hierarchical organization of commands, where each node represents, for example, a menu.
 */
static void build_command_tree()
{
    g_mgr.command_tree = t_command_node(L"root");

    for (const auto& action : g_mgr.actions)
    {
        std::vector<std::wstring> parts = split_action_path(action.params.path);

        t_command_node* current = &g_mgr.command_tree;

        for (const auto& part : parts)
        {
            auto it = std::ranges::find_if(current->children,
                                           [&](const t_command_node& node) {
                                               return node.name == part;
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

    size_t menu_id_counter = 0;
    iterate_all_children_and_self(g_mgr.command_tree, [&](t_command_node& node) {
        assert(node.menu_id <= IDM_RESERVED_END);
        node.menu_id = (uint16_t)menu_id_counter;
        menu_id_counter++;
    });

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
static void log_menu_structure(const t_command_node& node, size_t depth = 0)
{
    if (depth == 0)
    {
        g_view_logger->debug(L"---- Menu structure ----");
    }

    std::wstring indent(depth * 2, L' ');
    g_view_logger->debug(L"{} {} (ID: {})", indent, node.name, node.menu_id);

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
static void add_menu_items(const t_command_node& node, const HMENU parent_menu, const size_t depth = 0)
{
    for (const auto& command : node.children)
    {
        if (!command.children.empty())
        {
            HMENU new_menu = CreatePopupMenu();
            InsertMenu(parent_menu, GetMenuItemCount(parent_menu), MF_BYPOSITION | MF_POPUP, (UINT_PTR)new_menu, command.display_name.c_str());
            if (command.has_separator)
            {
                InsertMenu(parent_menu, GetMenuItemCount(parent_menu), MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
            }
            add_menu_items(command, new_menu, depth + 1);
            continue;
        }

        InsertMenu(parent_menu, GetMenuItemCount(parent_menu), MF_BYPOSITION | MF_STRING, command.menu_id, command.display_name.c_str());
        if (command.has_separator)
        {
            InsertMenu(parent_menu, GetMenuItemCount(parent_menu), MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
        }
    }
}

/**
 * \brief Sets the accelerator text for a menu item, replacing any existing accelerator text.
 */
static void set_menu_accelerator_text(const HMENU menu_bar, const uint16_t menu_id, const std::wstring& text)
{
    wchar_t str[256] = {0};
    GetMenuString(menu_bar, menu_id, str, std::size(str), MF_BYCOMMAND);

    std::wstring menu_text(str);

    // Remove any existing accelerator text
    const size_t tab_pos = menu_text.find(L'\t');
    if (tab_pos != std::wstring::npos)
        menu_text = menu_text.substr(0, tab_pos);

    // Append accelerator text if there's any
    if (!text.empty())
    {
        menu_text += L'\t';
        menu_text += text;
    }

    ModifyMenu(menu_bar, menu_id, MF_BYCOMMAND | MF_STRING, menu_id, menu_text.c_str());
}

/**
 * \brief Sets the accelerator text for a menu item based on the specified hotkey.
 */
static void set_menu_accelerator_text_from_hotkey(const HMENU menu_bar, const uint16_t menu_id, const Hotkey::t_hotkey& hotkey)
{
    const auto hotkey_str = hotkey.to_wstring();
    set_menu_accelerator_text(menu_bar, menu_id, hotkey.is_nothing() ? L"" : hotkey_str.c_str());
}

/**
 * \brief Updates the enabled states of all menu items using the value from the action enabled function.
 */
static void update_menu_enabled_states()
{
    const HMENU main_menu = GetMenu(g_main_hwnd);
    iterate_all_children_and_self(g_mgr.command_tree, [&](const t_command_node& node) {
        if (!node.action)
        {
            return;
        }
        const bool enabled = node.action->params.get_enabled();
        EnableMenuItem(main_menu, node.menu_id, enabled ? MF_ENABLED : MF_GRAYED);
    });
}

static void build_menu()
{
    build_command_tree();

    // 1. Delete all existing menu items
    const HMENU main_menu = GetMenu(g_main_hwnd);
    const auto menu_count = GetMenuItemCount(main_menu);
    for (int i = 0; i < menu_count; ++i)
    {
        DeleteMenu(main_menu, 0, MF_BYPOSITION);
    }

    // 2. Add the built-in commands first. We stomp the hierarchy down by one level for this, so we don't have a top-level menu called "Mupen64".
    // The built-in commands are assumed to be the first child of the root node in the command tree.
    add_menu_items(g_mgr.command_tree.children.at(0), main_menu);

    // 3. Add all other externally-registered commands
    t_command_node root_copy = g_mgr.command_tree;
    root_copy.children.erase(root_copy.children.begin());
    add_menu_items(root_copy, main_menu);

    // 4. Apply the accelerator text to all menu items.
    iterate_all_children_and_self(g_mgr.command_tree, [&](const t_command_node& node) {
        if (node.action)
        {
            const Hotkey::t_hotkey hotkey = g_config.hotkeys.contains(node.action->params.path) ? g_config.hotkeys[node.action->params.path] : Hotkey::t_hotkey{};
            set_menu_accelerator_text_from_hotkey(main_menu, node.menu_id, hotkey);
        }
    });

    // 5. Update the enabled states of all menu items.
    update_menu_enabled_states();
}
