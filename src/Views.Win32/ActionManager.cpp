/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ActionManager.h>
#include <Messenger.h>

using t_action_params = ActionManager::t_action_params;
using action_path = ActionManager::action_path;
using action_filter = ActionManager::action_filter;

struct t_action {
    t_action_params params{};
};

struct t_action_manager {
    std::vector<t_action> actions{};
    bool batched_work{};
};

static t_action_manager g_mgr{};

/**
 * \brief Normalizes a filter by deconstructing it into segments, then reconstructing it with a consistent format.
 */
static action_filter normalize_filter(const action_filter& filter)
{
    const auto parts = ActionManager::get_segments(filter);
    return io_service.join_wstring(parts, L">");
}

/**
 * \brief Finds all actions using the given filter.
 */
static std::vector<t_action*> get_action_ptrs_matching_filter(const action_filter& filter)
{
    const auto normalized_filter = normalize_filter(filter);

    for (auto& action : g_mgr.actions)
    {
        if (action.params.path == normalized_filter)
        {
            return {&action};
        }
    }

    const auto segments = ActionManager::get_segments(normalized_filter);

    std::vector<t_action*> actions;

    for (auto& action : g_mgr.actions)
    {
        const auto action_segments = ActionManager::get_segments(action.params.path);
        if (action_segments.size() >= segments.size() && std::equal(segments.begin(), segments.end(), action_segments.begin()))
        {
            actions.push_back(&action);
        }
    }

    return actions;
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
    action.params.path = normalize_filter(action.params.path);

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

bool ActionManager::remove(const action_filter& filter)
{
    const auto actions = get_action_ptrs_matching_filter(filter);

    if (actions.empty())
    {
        g_view_logger->error(L"ActionManager::remove: Action '{}' not found.", filter);
        return false;
    }

    // Call the on_removed callbacks first and before removing anything - we don't want weirdness if the callbacks do some bullshit like calling back into the ActionManager...
    for (const auto& action_to_be_removed : actions)
    {
        for (const auto& existing_action : g_mgr.actions)
        {
            if (existing_action.params.path != action_to_be_removed->params.path)
            {
                continue;
            }

            if (existing_action.params.on_removed)
            {
                existing_action.params.on_removed();
            }
        }
    }

    for (const auto& action_to_be_removed : actions)
    {
        std::erase_if(g_mgr.actions, [&](const t_action& a) {
            return a.params.path == action_to_be_removed->params.path;
        });
    }

    if (!g_mgr.batched_work)
    {
        Messenger::broadcast(Messenger::Message::ActionRegistryChanged, nullptr);
    }

    return true;
}

bool ActionManager::associate_hotkey(const action_path& path, const Hotkey::t_hotkey& hotkey, bool overwrite_existing)
{
    const auto normalized_path = normalize_filter(path);

    if (!validate_action_path(normalized_path))
    {
        g_view_logger->error(L"ActionManager::associate_hotkey: Malformed action path '{}'.", normalized_path);
        return false;
    }

    if (get_action_ptrs_matching_filter(normalized_path).empty())
    {
        g_view_logger->error(L"ActionManager::associate_hotkey: Action '{}' not found.", normalized_path);
        return false;
    }

    if (overwrite_existing)
    {
        if (!g_config.hotkeys.contains(normalized_path))
        {
            g_view_logger->debug(L"ActionManager::associate_hotkey: Initial hotkey registered for '{}': {}.", normalized_path, hotkey.to_wstring());
            g_config.inital_hotkeys[normalized_path] = hotkey;
        }

        g_view_logger->debug(L"ActionManager::associate_hotkey: Hotkey registered for '{}': {}.", normalized_path, hotkey.to_wstring());
        g_config.hotkeys[normalized_path] = hotkey;
    }
    else
    {
        if (!g_config.hotkeys.contains(normalized_path))
        {
            g_config.hotkeys[normalized_path] = hotkey;
        }
        else
        {
            g_view_logger->debug(L"ActionManager::associate_hotkey: {} is already registered, doing nothing.", normalized_path, hotkey.to_wstring());
        }
    }

    if (!g_mgr.batched_work)
    {
        Messenger::broadcast(Messenger::Message::ActionRegistryChanged, nullptr);
    }

    return true;
}

bool ActionManager::get_action_enabled(const action_path& path)
{
    const auto actions = get_action_ptrs_matching_filter(path);

    if (actions.empty())
    {
        g_view_logger->error(L"ActionManager::is_action_enabled: Action '{}' not found.", path);
        return false;
    }

    if (actions.size() > 1)
    {
        g_view_logger->error(L"ActionManager::is_action_enabled: Expected fully-qualified path, but got partially-qualified one.");
        return false;
    }

    const auto action = actions.front();

    if (action->params.get_enabled)
    {
        return action->params.get_enabled();
    }

    return true;
}

bool ActionManager::get_action_active(const action_path& path)
{
    const auto actions = get_action_ptrs_matching_filter(path);

    if (actions.empty())
    {
        g_view_logger->error(L"ActionManager::is_action_active: Action '{}' not found.", path);
        return false;
    }

    if (actions.size() > 1)
    {
        g_view_logger->error(L"ActionManager::is_action_active: Expected fully-qualified path, but got partially-qualified one.");
        return false;
    }

    const auto action = actions.front();

    if (action->params.get_active)
    {
        return action->params.get_active();
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

void ActionManager::notify_enabled_changed(const action_filter& filter)
{
    const auto actions = get_action_ptrs_matching_filter(filter);
    Messenger::broadcast(Messenger::Message::ActionEnabledChanged, actions);
}

void ActionManager::notify_active_changed(const action_filter& filter)
{
    const auto actions = get_action_ptrs_matching_filter(filter);
    Messenger::broadcast(Messenger::Message::ActionActiveChanged, actions);
}

void ActionManager::notify_display_name_changed(const action_filter& filter)
{
    const auto actions = get_action_ptrs_matching_filter(filter);
    Messenger::broadcast(Messenger::Message::ActionRealNameChanged, actions);
}

std::wstring ActionManager::get_display_name(const action_filter& filter, bool ignore_override)
{
    const auto normalized_path = normalize_filter(filter);

    const auto actions = get_action_ptrs_matching_filter(normalized_path);

    if (actions.empty() || actions.size() > 1)
    {
        // It's a filter, not a fully-qualified action path. We don't look up anything, but just do some formatting instead.

        auto name = get_segments(filter).back();
        const auto has_separator = name.ends_with(SEPARATOR_SUFFIX);

        if (has_separator)
        {
            name = name.substr(0, name.size() - SEPARATOR_SUFFIX.size());
        }

        return name;
    }

    const auto action = actions.front();

    const auto segments = get_segments(normalized_path);
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

    if (action->params.get_display_name && !ignore_override)
    {
        const auto override_display_name = action->params.get_display_name();
        if (!override_display_name.empty())
        {
            display_name = override_display_name;
        }
    }

    return display_name;
}

std::vector<action_path> ActionManager::get_actions_matching_filter(const action_filter& filter)
{
    std::vector<action_path> result;
    result.reserve(g_mgr.actions.size());

    if (filter.empty())
    {
        for (const auto& action : g_mgr.actions)
        {
            result.emplace_back(action.params.path);
        }
        return result;
    }

    const auto actions = get_action_ptrs_matching_filter(filter);
    for (const auto& action : actions)
    {
        result.push_back(action->params.path);
    }

    return result;
}

std::vector<action_path> ActionManager::get_segments(const action_filter& filter)
{
    std::vector<action_path> parts = io_service.split_wstring(filter, L">");
    for (auto& part : parts)
    {
        part = io_service.trim(part);
    }

    std::erase_if(parts, [](const std::wstring& part) {
        return part.empty();
    });

    return parts;
}

void ActionManager::invoke(const action_path& path, const bool up)
{
    const auto normalized_path = normalize_filter(path);

    if (!validate_action_path(normalized_path))
    {
        g_view_logger->error(L"ActionManager::invoke: Malformed action path '{}'.", normalized_path);
        return;
    }

    const auto actions = get_action_ptrs_matching_filter(normalized_path);

    if (actions.empty())
    {
        g_view_logger->error(L"ActionManager::invoke: Action with path '{}' not found.", normalized_path);
        return;
    }

    if (actions.size() > 1)
    {
        g_view_logger->error(L"ActionManager::invoke: Expected fully-qualified path, but got partially-qualified one.");
        return;
    }

    const auto action = actions.front();

    if (action->params.get_enabled && !action->params.get_enabled())
    {
        return;
    }

    if (!up)
    {
        action->params.down_callback();
    }
    else
    {
        if (action->params.up_callback)
        {
            action->params.up_callback();
        }
    }
}
