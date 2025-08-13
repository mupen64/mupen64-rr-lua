/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ActionManager.h>
#include <Messenger.h>
#include <components/ActionMenu.h>

const auto MANAGED_MENU_CTX = L"Mupen64_ManagedMenuContext";

struct t_menu_item {
    uint16_t id{};
    size_t position_under_parent{};
    HMENU popup_handle{};
    HMENU parent_menu{};
    bool has_menu{};

    std::wstring action_path{};
    std::vector<t_menu_item> children{};

private:
    std::wstring m_path{};
    bool m_has_separator{};

public:
    explicit t_menu_item(const std::wstring& path);

    /**
     * \brief Performs a depth-first iteration over the menu item tree, applying the given action to each item. The action is also applied to the initial item itself.
     */
    void iterate_children_and_self(const std::function<void(t_menu_item& item)>& action);

    [[nodiscard]] const auto& raw_path() const
    {
        return m_path;
    }

    [[nodiscard]] const bool& has_separator() const
    {
        return m_has_separator;
    }
};

struct t_action_menu_context {
    HWND hwnd{};
    HMENU menu_bar{};
    t_menu_item menu{L"Root"};
    size_t menu_id_counter{};
    std::set<std::wstring> enabled_state_invalidated_actions{};
    std::set<std::wstring> active_state_invalidated_actions{};
    std::set<std::wstring> display_name_invalidated_actions{};
};

struct t_action_menu_global_context {
    std::vector<t_action_menu_context*> active_contexts{};
    std::vector<std::wstring> actions{};
};

static t_action_menu_global_context g_am_ctx{};

t_menu_item::t_menu_item(const std::wstring& path)
{
    this->m_path = path;

    const auto name = ActionManager::get_segments(path).back();
    this->m_has_separator = name.ends_with(ActionManager::SEPARATOR_SUFFIX);
}

void t_menu_item::iterate_children_and_self(const std::function<void(t_menu_item& item)>& action)
{
    action(*this);
    for (auto& child : children)
    {
        child.iterate_children_and_self(action);
    }
}

/**
 * \brief Walks the command tree to find the command item corresponding to the "Name" segment of the fully-qualified action path.
 */
static t_menu_item* find_item_by_path(t_action_menu_context& ctx, const std::wstring& path)
{
    t_menu_item* found_item = nullptr;

    ctx.menu.iterate_children_and_self([&](t_menu_item& item) {
        if (item.raw_path() == path)
        {
            found_item = &item;
        }
    });

    return found_item;
}

/**
 * \brief Gets the effective display name for the given menu item, including any accelerator text if applicable.
 */
static std::wstring get_display_name(const t_menu_item& item)
{
    auto display_name = ActionManager::get_display_name(item.raw_path());

    // Add the accelerator text if there is any :P
    if (!item.action_path.empty() && g_config.hotkeys.contains(item.action_path))
    {
        const auto hotkey = g_config.hotkeys[item.action_path];
        if (!hotkey.is_nothing())
        {
            display_name += std::format(L"\t{}", hotkey.to_wstring());
        }
    }

    return display_name;
}

/**
 * \brief Updates the display names of the specified menu items.
 */
static void update_display_names(t_action_menu_context& ctx, const std::set<std::wstring>& actions)
{
    MENUITEMINFO mii{};
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STRING;

    for (const auto& action : actions)
    {
        const auto item = find_item_by_path(ctx, action);
        if (!item)
        {
            continue;
        }
        if (!item->has_menu)
        {
            return;
        }

        const auto display_name = get_display_name(*item);

        mii.dwTypeData = const_cast<LPWSTR>(display_name.c_str());
        mii.cch = display_name.length();

        if (!SetMenuItemInfo(item->parent_menu, item->position_under_parent, TRUE, &mii))
        {
            g_view_logger->error(L"ActionManager::update_menu_names: Couldn't update name of '{}'.", display_name);
        }
    }
}

/**
 * \brief Updates the enabled states of the specified menu items.
 */
static void update_enabled_states(t_action_menu_context& ctx, const std::set<std::wstring>& actions)
{
    for (const auto& action : actions)
    {
        const auto item = find_item_by_path(ctx, action);
        if (!item)
        {
            continue;
        }

        const bool enabled = ActionManager::get_enabled(action);
        EnableMenuItem(ctx.menu_bar, item->id, enabled ? MF_ENABLED : MF_GRAYED);
    }
}

/**
 * \brief Updates the active states of the specified menu items.
 */
static void update_active_states(t_action_menu_context& ctx, const std::set<std::wstring>& actions)
{
    for (const auto& action : actions)
    {
        const auto item = find_item_by_path(ctx, action);
        if (!item)
        {
            continue;
        }

        const bool active = ActionManager::get_active(action);
        CheckMenuItem(ctx.menu_bar, item->id, active ? MF_CHECKED : MF_UNCHECKED);
    }
}

static bool handle_menu_interaction(t_action_menu_context& ctx, const size_t id)
{
    std::wstring found_action_path;
    ctx.menu.iterate_children_and_self([&](const t_menu_item& item) {
        if (item.id == id)
        {
            found_action_path = item.action_path;
        }
    });

    if (found_action_path.empty())
    {
        return false;
    }

    ActionManager::invoke(found_action_path);

    return true;
}

/**
 * \brief Builds the initial menu tree based on the registered actions' paths.
 */
static void build_initial_menu_tree(t_action_menu_context& ctx)
{
    ctx.menu = t_menu_item(L"Root");

    for (const auto& path : g_am_ctx.actions)
    {
        std::vector<std::wstring> parts = ActionManager::get_segments(path);
        std::wstring path_up_to_here;
        path_up_to_here.reserve(parts.size() * 20);

        t_menu_item* current = &ctx.menu;

        for (size_t i = 0; i < parts.size(); ++i)
        {
            path_up_to_here += parts[i];

            const auto it = std::ranges::find_if(current->children,
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

            if (i == parts.size() - 1)
            {
                current->action_path = path;
            }

            path_up_to_here += ActionManager::SEGMENT_SEPARATOR;
        }
    }
}

/**
 * \brief Adds menu items to the specified parent menu based on the command tree structure.
 */
static void add_menu_items(t_action_menu_context& ctx, t_menu_item& item, const HMENU parent_menu)
{
    ctx.menu_id_counter++;
    runtime_assert(ctx.menu_id_counter <= IDM_RESERVED_END, std::format(L"Menu ID counter overflow: {} (max {})", ctx.menu_id_counter, IDM_RESERVED_END));

    item.id = (uint16_t)ctx.menu_id_counter;
    item.parent_menu = parent_menu;
    item.position_under_parent = GetMenuItemCount(parent_menu);
    item.has_menu = true;

    const bool has_action = !item.action_path.empty();

    const auto display_name = get_display_name(item);
    const auto enabled = has_action ? ActionManager::get_enabled(item.raw_path()) : true;
    const auto active = has_action ? ActionManager::get_active(item.raw_path()) : true;

    auto initialize_menu_item_state = [&] {
        if (!enabled)
        {
            EnableMenuItem(parent_menu, item.id, MF_DISABLED);
        }

        if (active)
        {
            CheckMenuItem(parent_menu, item.id, MF_CHECKED);
        }
    };

    if (item.children.empty())
    {
        AppendMenu(parent_menu, MF_STRING, item.id, display_name.c_str());
        initialize_menu_item_state();

        if (item.has_separator())
        {
            AppendMenu(parent_menu, MF_SEPARATOR, 0, nullptr);
        }

        return;
    }

    item.popup_handle = CreatePopupMenu();
    AppendMenu(parent_menu, MF_STRING | MF_POPUP, (UINT_PTR)item.popup_handle, display_name.c_str());
    initialize_menu_item_state();

    if (item.has_separator())
    {
        AppendMenu(parent_menu, MF_SEPARATOR, 0, nullptr);
    }

    for (auto& child : item.children)
    {
        add_menu_items(ctx, child, item.popup_handle);
    }
}

/**
 * \brief Deletes the current menu of the context's window and sets it to a new menu.
 */
static void reset_menu(t_action_menu_context& ctx)
{
    const HMENU prev_menu = GetMenu(ctx.hwnd);

    ctx.menu_bar = CreateMenu();
    SetMenu(ctx.hwnd, ctx.menu_bar);

    if (IsMenu(prev_menu))
        DestroyMenu(prev_menu);
}

static void build_menu(t_action_menu_context& ctx)
{
    SetWindowRedraw(ctx.hwnd, false);

    reset_menu(ctx);

    build_initial_menu_tree(ctx);

    if (!ctx.menu.children.empty())
    {
        ctx.menu_id_counter = 0;
        for (auto& item : ctx.menu.children.at(0).children)
        {
            add_menu_items(ctx, item, ctx.menu_bar);
        }
        for (size_t i = 1; i < ctx.menu.children.size(); ++i)
        {
            add_menu_items(ctx, ctx.menu.children[i], ctx.menu_bar);
        }
    }

    SetWindowRedraw(ctx.hwnd, true);
    DrawMenuBar(ctx.hwnd);
}

static LRESULT CALLBACK action_menu_wnd_subclass_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR dwRefData)
{
    auto ctx = static_cast<t_action_menu_context*>(GetProp(hwnd, MANAGED_MENU_CTX));

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, action_menu_wnd_subclass_proc, sId);
        RemoveProp(hwnd, MANAGED_MENU_CTX);
        std::erase_if(g_am_ctx.active_contexts, [=](const auto& other_ctx) {
            return ctx == other_ctx;
        });
        delete ctx;
        ctx = nullptr;
        break;
    case WM_INITMENU:
        {
            update_enabled_states(*ctx, ctx->enabled_state_invalidated_actions);
            update_active_states(*ctx, ctx->active_state_invalidated_actions);
            update_display_names(*ctx, ctx->display_name_invalidated_actions);
            DrawMenuBar(ctx->hwnd);

            ctx->enabled_state_invalidated_actions.clear();
            ctx->active_state_invalidated_actions.clear();
            ctx->display_name_invalidated_actions.clear();
            break;
        }
    case WM_COMMAND:
        if (handle_menu_interaction(*ctx, LOWORD(wParam)))
        {
            return 0;
        }
        break;
    default:
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ActionMenu::init()
{
    Messenger::subscribe(Messenger::Message::ActionRegistryChanged, [](const auto& any) {
        g_am_ctx.actions = ActionManager::get_actions_matching_filter(L"*");
        for (const auto& ctx : g_am_ctx.active_contexts)
        {
            build_menu(*ctx);
        }
    });

    // OPTIMIZATION: Instead of updating **all** menu item states on WM_INITMENU,
    // we subscribe to the change notifications and store a set of "invalidated" menu items and only update the changed items in WM_INITMENU.
    Messenger::subscribe(Messenger::Message::ActionDisplayNameChanged, [](const auto& any) {
        const auto actions = std::any_cast<std::vector<std::wstring>>(any);
        for (const auto& action : actions)
        {
            for (const auto& ctx : g_am_ctx.active_contexts)
            {
                ctx->display_name_invalidated_actions.insert(action);
            }
        }
    });

    Messenger::subscribe(Messenger::Message::ActionEnabledChanged, [](const auto& any) {
        const auto actions = std::any_cast<std::vector<std::wstring>>(any);
        for (const auto& action : actions)
        {
            for (const auto& ctx : g_am_ctx.active_contexts)
            {
                ctx->enabled_state_invalidated_actions.insert(action);
            }
        }
    });

    Messenger::subscribe(Messenger::Message::ActionActiveChanged, [](const auto& any) {
        const auto actions = std::any_cast<std::vector<std::wstring>>(any);
        for (const auto& action : actions)
        {
            for (const auto& ctx : g_am_ctx.active_contexts)
            {
                ctx->active_state_invalidated_actions.insert(action);
            }
        }
    });
}

bool ActionMenu::add_managed_menu(const HWND hwnd)
{
    auto context = new t_action_menu_context();
    context->hwnd = hwnd;
    g_am_ctx.active_contexts.push_back(context);

    SetProp(hwnd, MANAGED_MENU_CTX, context);

    SetWindowSubclass(hwnd, action_menu_wnd_subclass_proc, 0, reinterpret_cast<DWORD_PTR>(context));

    return true;
}
