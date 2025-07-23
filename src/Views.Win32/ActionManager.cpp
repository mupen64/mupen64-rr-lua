/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ActionManager.h>

struct t_action_manager {
    std::vector<ActionManager::t_action> actions{};
    std::unordered_map<std::wstring, int32_t> action_name_menu_map{};
    ActionManager::t_command_node command_tree{L"root"};
};

static t_action_manager g_mgr{};

static void build_menu();

static void iterate_all_children_and_self(ActionManager::t_command_node& node, const std::function<void(ActionManager::t_command_node& node)>& predicate)
{
    predicate(node);
    for (auto& child : node.children)
    {
        iterate_all_children_and_self(child, predicate);
    }
}

static ActionManager::t_command_node* find_command_node_by_name(const std::wstring& name)
{
    const std::wstring last_segment = io_service.trim(name.substr(name.find_last_of(L'>') + 1));

    ActionManager::t_command_node* found_node = nullptr;
    iterate_all_children_and_self(g_mgr.command_tree, [&](ActionManager::t_command_node& node) {
        if (node.name == last_segment)
        {
            found_node = &node;
        }
    });

    return found_node;
}

std::wstring ActionManager::t_hotkey::to_wstring() const
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

void ActionManager::add(t_action action)
{
    std::erase_if(g_mgr.actions, [&](const t_action& a) {
        return a.name == action.name;
    });

    g_mgr.actions.emplace_back(action);

    build_menu();
}

std::optional<std::reference_wrapper<ActionManager::t_action>> ActionManager::get_by_name(const std::wstring& name)
{
    for (auto& action : g_mgr.actions)
    {
        if (action.name == name)
        {
            return action;
        }
    }
    return std::nullopt;
}

std::vector<ActionManager::t_action> ActionManager::get_actions()
{
    return g_mgr.actions;
}

bool ActionManager::handle_menu_interaction(size_t id)
{
    g_view_logger->info(L"interaction {}", id);

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

    g_view_logger->info(L"interaction >>> {}", action->name);

    action->down_callback();

    return true;
}

static void set_menu_accelerator(int element_id, const wchar_t* acc)
{
    wchar_t string[256] = {0};
    GetMenuString(GetMenu(g_main_hwnd), element_id, string, std::size(string), MF_BYCOMMAND);

    auto tab = wcschr(string, '\t');
    if (tab)
        *tab = '\0';
    if (StrCmp(acc, L""))
        wsprintf(string, L"%s\t%s", string, acc);

    ModifyMenu(GetMenu(g_main_hwnd), element_id, MF_BYCOMMAND | MF_STRING, element_id, string);
}

static void set_hotkey_menu_accelerators(const ActionManager::t_hotkey& hotkey, const int menu_item_id)
{
    const auto hotkey_str = hotkey.to_wstring();
    set_menu_accelerator(menu_item_id, hotkey_str == L"(nothing)" ? L"" : hotkey_str.c_str());
}

/**
 * \brief Builds a tree structure containing command nodes based on the currently registered actions' names.
 * The tree structure allows for hierarchical organization of commands, where each node represents, for example, a menu.
 */
static void build_command_tree()
{
    g_mgr.command_tree = ActionManager::t_command_node{L"root"};

    for (const auto& action : g_mgr.actions)
    {
        std::vector<std::wstring> parts = io_service.split_wstring(action.name, L">");
        for (auto& part : parts)
        {
            part = io_service.trim(part);
        }

        ActionManager::t_command_node* current = &g_mgr.command_tree;

        for (const auto& part : parts)
        {
            auto it = std::ranges::find_if(current->children,
                                           [&](const ActionManager::t_command_node& node) {
                                               return node.name == part;
                                           });

            if (it != current->children.end())
            {
                current = &*it;
            }
            else
            {
                current->children.push_back(ActionManager::t_command_node{part});
                current = &current->children.back();
            }
        }
    }

    size_t menu_id_counter = 0;
    iterate_all_children_and_self(g_mgr.command_tree, [&](ActionManager::t_command_node& node) {
        assert(node.menu_id <= IDM_RESERVED_END);
        node.menu_id = (uint16_t)menu_id_counter;
        menu_id_counter++;
    });

    for (auto& action : g_mgr.actions)
    {
        const auto command = find_command_node_by_name(action.name);
        if (!command)
        {
            g_view_logger->error(L"Failed to find command node for action: {}", action.name);
            continue;
        }
        command->action = &action;
    }
}

/**
 * \brief Logs the structure of the command tree to the logger.
 */
static void log_menu_structure(const ActionManager::t_command_node& node, size_t depth = 0)
{
    if (depth == 0)
    {
        g_view_logger->info(L"---- Menu structure ----");
    }

    std::wstring indent(depth * 2, L' ');
    g_view_logger->info(L"{} {} (ID: {})", indent, node.name, node.menu_id);

    for (const auto& child : node.children)
    {
        log_menu_structure(child, depth + 1);
    }

    if (depth == 0)
    {
        g_view_logger->info(L"---- End of menu structure ----");
    }
}

/**
 * \brief Adds menu items to the specified parent menu based on the command tree structure.
 */
static void add_menu_items(const ActionManager::t_command_node& node, const HMENU parent_menu, const size_t depth = 0)
{
    for (const auto& command : node.children)
    {
        if (!command.children.empty())
        {
            // Special case: if this isn't a
            HMENU new_menu = CreatePopupMenu();
            InsertMenu(parent_menu, GetMenuItemCount(parent_menu), MF_BYPOSITION | MF_POPUP, (UINT_PTR)new_menu, command.name.c_str());
            add_menu_items(command, new_menu, depth + 1);
            continue;
        }

        // For leaves, we just add a normal menu item
        InsertMenu(parent_menu, GetMenuItemCount(parent_menu), MF_BYPOSITION | MF_STRING, command.menu_id, command.name.c_str());
    }
}

static void build_menu()
{
    build_command_tree();

    log_menu_structure(g_mgr.command_tree);

    // 1. Delete all existing menu items
    HMENU main_menu = GetMenu(g_main_hwnd);
    const auto menu_count = GetMenuItemCount(main_menu);
    for (int i = 0; i < menu_count; ++i)
    {
        DeleteMenu(main_menu, 0, MF_BYPOSITION);
    }

    // 2. Add the built-in commands first. We stomp the hierarchy down by one level for this, so we don't have a top-level menu called "Mupen64".
    // The built-in commands are assumed to be the first child of the root node in the command tree.
    add_menu_items(g_mgr.command_tree.children.at(0), main_menu);

    // 3. Add all other externally-registered commands
    ActionManager::t_command_node root_copy = g_mgr.command_tree;
    root_copy.children.erase(root_copy.children.begin());
    add_menu_items(root_copy, main_menu);
}
