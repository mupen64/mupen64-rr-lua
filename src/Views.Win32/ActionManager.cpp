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
 * \brief Finds all actions using the given filter.
 */
static std::vector<t_action*> get_action_ptrs_matching_filter(const action_filter& filter)
{
    const auto normalized_filter = ActionManager::normalize_filter(filter);
    std::vector<t_action*> result;

    // Special case: pure wildcard filter, matches everything.
    if (normalized_filter == L"*")
    {
        result.reserve(g_mgr.actions.size());
        for (auto& action : g_mgr.actions)
        {
            result.emplace_back(&action);
        }
        return result;
    }

    const auto filter_segments = ActionManager::get_segments(normalized_filter);
    if (filter_segments.empty())
    {
        return result;
    }

    const bool has_wildcard = filter_segments.back() == L"*";
    const size_t filter_segments_to_compare = has_wildcard ? filter_segments.size() - 1 : filter_segments.size();

    for (auto& action : g_mgr.actions)
    {
        const auto path_segments = ActionManager::get_segments(action.params.path);

        if (has_wildcard)
        {
            // The path must have more segments than the filter if the filter ends with a wildcard, otherwise we aren't deep enough.
            if (path_segments.size() <= filter_segments_to_compare)
            {
                continue;
            }
        }
        else
        {
            if (path_segments.size() != filter_segments_to_compare)
            {
                continue;
            }
        }

        bool is_match = true;
        for (size_t i = 0; i < filter_segments_to_compare; ++i)
        {
            if (path_segments[i] != filter_segments[i])
            {
                is_match = false;
                break;
            }
        }

        if (is_match)
        {
            result.emplace_back(&action);
        }
    }

    return result;
}

/**
 * \brief Tries to resolve a fully-qualified action path to a single action pointer.
 */
static t_action* get_single_action_ptr_matching_path(const action_path& path)
{
    if (path.contains(L"*"))
    {
        g_view_logger->error(L"ActionManager::get_single_action_ptr_matching_filter: Expected path without wildcard.");
        return nullptr;
    }

    const auto actions = get_action_ptrs_matching_filter(path);

    if (actions.empty())
    {
        g_view_logger->error(L"ActionManager::get_single_action_ptr_matching_filter: Action not found.");
        return nullptr;
    }

    if (actions.size() > 1)
    {
        g_view_logger->error(L"ActionManager::get_single_action_ptr_matching_filter: Expected filter to resolve to only one action.");
        return nullptr;
    }

    return actions.front();
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

std::vector<action_path> ActionManager::remove(const action_filter& filter)
{
    const auto actions = get_action_ptrs_matching_filter(filter);

    std::vector<action_path> removed_paths;
    removed_paths.reserve(actions.size());

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

            removed_paths.emplace_back(existing_action.params.path);
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

    return removed_paths;
}

bool ActionManager::associate_hotkey(const action_path& path, const Hotkey::t_hotkey& hotkey, bool overwrite_existing)
{
    t_action* action = get_single_action_ptr_matching_path(path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::associate_hotkey: '{}' didn't resolve to an action", path);
        return false;
    }

    const auto normalized_path = action->params.path;

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
            g_config.inital_hotkeys[normalized_path] = hotkey;
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
    t_action* action = get_single_action_ptr_matching_path(path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::get_action_enabled: '{}' didn't resolve to an action", path);
        return false;
    }

    if (action->params.get_enabled)
    {
        return action->params.get_enabled();
    }

    return true;
}

bool ActionManager::get_action_active(const action_path& path)
{
    t_action* action = get_single_action_ptr_matching_path(path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::get_action_active: '{}' didn't resolve to an action", path);
        return false;
    }

    if (action->params.get_active)
    {
        return action->params.get_active();
    }

    return false;
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
    Messenger::broadcast(Messenger::Message::ActionDisplayNameChanged, actions);
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
            name = io_service.trim(name.substr(0, name.size() - SEPARATOR_SUFFIX.size()));
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
    const auto actions = get_action_ptrs_matching_filter(filter);

    std::vector<action_path> result;
    result.reserve(actions.size());

    for (const auto& action : actions)
    {
        result.emplace_back(action->params.path);
    }

    return result;
}

std::vector<action_filter> ActionManager::get_segments(const action_filter& filter)
{
    std::vector<action_filter> parts = io_service.split_wstring(filter, SEGMENT_SEPARATOR);
    for (auto& part : parts)
    {
        part = io_service.trim(part);
    }

    std::erase_if(parts, [](const std::wstring& part) {
        return part.empty();
    });

    return parts;
}

ActionManager::action_filter ActionManager::normalize_filter(const action_filter& filter)
{
    const auto parts = get_segments(filter);
    return io_service.join_wstring(parts, SEGMENT_SEPARATOR);
}

void ActionManager::invoke(const action_path& path, const bool up)
{
    t_action* action = get_single_action_ptr_matching_path(path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::invoke: '{}' didn't resolve to an action", path);
        return;
    }

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
