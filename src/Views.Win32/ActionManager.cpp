/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ActionManager.h>
#include <Messenger.h>

using t_action_params = ActionManager::t_action_params;
using t_action = ActionManager::t_action;

struct t_action_manager {
    std::vector<t_action> actions{};
    bool batched_work{};
};

static t_action_manager g_mgr{};

/**
 * \brief Normalizes an action's path by deconstructing it into segments, then reconstructing it with a consistent format.
 */
static std::wstring normalize_path(const std::wstring& path)
{
    const auto parts = ActionManager::get_path_segments(path);
    return io_service.join_wstring(parts, L">");
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

std::wstring t_action::display_name() const
{
    const auto normalized_path = normalize_path(params.path);
    const auto segments = get_path_segments(normalized_path);
    const auto& name = segments.back();
    const bool has_separator = name.ends_with(SEPARATOR_SUFFIX);

    std::wstring display_name;

    if (has_separator)
    {
        display_name = name.substr(0, name.size() - SEPARATOR_SUFFIX.size());
    }
    else
    {
        display_name = name;
    }

    if (params.get_real_name)
    {
        const auto real_name = params.get_real_name();
        if (!real_name.empty())
        {
            display_name = real_name;
        }
    }

    return display_name;
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
        Messenger::broadcast(Messenger::Message::ActionRegistryChanged, nullptr);
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
        Messenger::broadcast(Messenger::Message::ActionRegistryChanged, nullptr);
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
    Messenger::broadcast(Messenger::Message::ActionRegistryChanged, nullptr);
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
    Messenger::broadcast(Messenger::Message::ActionEnabledChanged, action);
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
    Messenger::broadcast(Messenger::Message::ActionActiveChanged, action);
}

void ActionManager::notify_real_name_changed(const std::wstring& path)
{
    const auto normalized_path = normalize_path(path);

    if (!validate_action_path(normalized_path))
    {
        g_view_logger->error(L"ActionManager::notify_real_name_changed: Malformed action path '{}'.", normalized_path);
        return;
    }

    t_action* action = find_action_by_path(normalized_path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::notify_real_name_changed: Action '{}' not found.", normalized_path);
        return;
    }

    g_view_logger->debug(L"ActionManager::notify_real_name_changed: Action '{}' real name changed.", normalized_path);
    Messenger::broadcast(Messenger::Message::ActionRealNameChanged, action);
}

std::wstring ActionManager::get_display_name(const std::wstring& path)
{
    const auto normalized_path = normalize_path(path);

    const auto item = find_action_by_path(normalized_path);

    if (!item)
    {
        // Probably an unqualified path, we go the other route
        auto name = get_path_segments(path).back();
        const auto has_separator = name.ends_with(SEPARATOR_SUFFIX);

        if (has_separator)
        {
            name = name.substr(0, name.size() - SEPARATOR_SUFFIX.size());
        }

        return name;
    }

    return item->display_name();
}

std::vector<t_action> ActionManager::get_actions()
{
    return g_mgr.actions;
}

std::vector<std::wstring> ActionManager::get_path_segments(const std::wstring& path)
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
