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
     * \brief Performs a depth-first iteration over the menu item tree, applying the given predicate to each item. The predicate is also applied to the initial item itself.
     */
    void iterate_children_and_self(const std::function<void(t_menu_item& item)>& predicate);

    [[nodiscard]] auto raw_path() const
    {
        return m_path;
    }

    [[nodiscard]] bool has_separator() const
    {
        return m_has_separator;
    }
};

struct t_action_menu_context {
    HWND hwnd{};
    t_menu_item menu{L"Root"};
    size_t menu_id_counter{};
};

struct t_action_menu_global_context {
    std::vector<t_action_menu_context*> active_contexts{};
    std::vector<std::wstring> actions{};
};

static t_action_menu_global_context g_am_ctx{};

t_menu_item::t_menu_item(const std::wstring& path)
{
    this->m_path = path;

    const auto name = ActionManager::get_path_segments(path).back();
    this->m_has_separator = name.ends_with(ActionManager::SEPARATOR_SUFFIX);
}

void t_menu_item::iterate_children_and_self(const std::function<void(t_menu_item& item)>& predicate)
{
    predicate(*this);
    for (auto& child : children)
    {
        child.iterate_children_and_self(predicate);
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
 * \brief Updates the enabled states of all menu items.
 */
static void update_menu_enabled_states(t_action_menu_context& ctx)
{
    const HMENU main_menu = GetMenu(ctx.hwnd);
    ctx.menu.iterate_children_and_self([&](const t_menu_item& item) {
        if (item.action_path.empty())
        {
            return;
        }

        const bool enabled = ActionManager::is_action_enabled(item.action_path);
        EnableMenuItem(main_menu, item.id, enabled ? MF_ENABLED : MF_GRAYED);
    });
}

/**
 * \brief Updates the active states of all menu items.
 */
static void update_menu_active_states(t_action_menu_context& ctx)
{
    const HMENU main_menu = GetMenu(ctx.hwnd);
    ctx.menu.iterate_children_and_self([&](const t_menu_item& item) {
        if (item.action_path.empty())
        {
            return;
        }

        const bool active = ActionManager::is_action_active(item.action_path);
        CheckMenuItem(main_menu, item.id, active ? MF_CHECKED : MF_UNCHECKED);
    });
}

/**
 * \brief Updates the names of all menu items.
 */
static void update_menu_names(t_action_menu_context& ctx)
{
    ctx.menu.iterate_children_and_self([&](const t_menu_item& item) {
        if (!item.has_menu)
        {
            return;
        }

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

static bool handle_menu_interaction(t_action_menu_context& ctx, size_t id)
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
        std::vector<std::wstring> parts = ActionManager::get_path_segments(path);

        t_menu_item* current = &ctx.menu;

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

    for (auto& path : g_am_ctx.actions)
    {
        const auto item = find_item_by_path(ctx, path);
        runtime_assert(item, std::format(L"ActionManager::build_initial_menu_tree: Action '{}' has no node.", path));
        item->action_path = path;
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

    if (item.children.empty())
    {
        AppendMenu(parent_menu, MF_STRING, item.id, L" ");

        if (item.has_separator())
        {
            AppendMenu(parent_menu, MF_SEPARATOR, 0, nullptr);
        }
        return;
    }

    item.popup_handle = CreatePopupMenu();
    AppendMenu(parent_menu, MF_STRING | MF_POPUP, (UINT_PTR)item.popup_handle, L" ");

    if (item.has_separator())
    {
        AppendMenu(parent_menu, MF_SEPARATOR, 0, nullptr);
    }

    for (auto& child : item.children)
    {
        add_menu_items(ctx, child, item.popup_handle);
    }
}

static void reset_menu(t_action_menu_context& ctx)
{
    if (!IsMenu(GetMenu(ctx.hwnd)))
    {
        SetMenu(ctx.hwnd, CreateMenu());
    }

    const HMENU main_menu_bar = GetMenu(ctx.hwnd);
    const auto menu_count = GetMenuItemCount(main_menu_bar);
    for (int i = 0; i < menu_count; ++i)
    {
        DeleteMenu(main_menu_bar, 0, MF_BYPOSITION);
    }
}

static void build_menu(t_action_menu_context& ctx)
{
    g_am_ctx.actions = ActionManager::get_actions_matching_filter();

    reset_menu(ctx);

    build_initial_menu_tree(ctx);

    const HMENU main_menu_bar = GetMenu(ctx.hwnd);

    ctx.menu_id_counter = 0;
    for (auto& item : ctx.menu.children.at(0).children)
    {
        add_menu_items(ctx, item, main_menu_bar);
    }
    for (size_t i = 1; i < ctx.menu.children.size(); ++i)
    {
        for (auto& item : ctx.menu.children[i].children)
        {
            add_menu_items(ctx, item, main_menu_bar);
        }
    }

    // 3. Update all the stuff relevant to the menu.
    update_menu_enabled_states(ctx);
    update_menu_active_states(ctx);
    update_menu_names(ctx);

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
        update_menu_enabled_states(*ctx);
        update_menu_active_states(*ctx);
        update_menu_names(*ctx);
        DrawMenuBar(ctx->hwnd);
        break;
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

static void action_registry_changed()
{
    for (const auto& ctx : g_am_ctx.active_contexts)
    {
        build_menu(*ctx);
    }
}

void ActionMenu::init()
{
    Messenger::subscribe(Messenger::Message::ActionRegistryChanged, [](const auto& any) {
        action_registry_changed();
    });

    // NOTE: We don't handle ActionEnabledChanged/ActionActiveChanged/ActionRealNameChanged here because we update the menu in-place in WM_INITMENU
}

bool ActionMenu::add_managed_menu(const HWND hwnd)
{
    auto context = new t_action_menu_context();
    context->hwnd = hwnd;
    g_am_ctx.active_contexts.push_back(context);

    SetProp(hwnd, MANAGED_MENU_CTX, context);

    SetWindowSubclass(hwnd, action_menu_wnd_subclass_proc, 0, reinterpret_cast<DWORD_PTR>(context));

    build_menu(*context);

    return true;
}
