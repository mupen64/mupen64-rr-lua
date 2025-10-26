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

struct t_action
{
    t_action_params params{};

    std::wstring raw_name{};
    std::vector<std::wstring> segments{};

    std::optional<std::wstring> display_name{};
    std::optional<bool> enabled{};
    std::optional<bool> active{};

    bool pressed{};
};

struct t_action_manager
{
    std::vector<t_action> actions{};
    bool batched_work{};
    bool lock_hotkeys{};
    MicroLRU::Cache<action_filter, std::vector<std::wstring>> segment_cache{256,
                                                                            [](const std::vector<std::wstring> &) {}};
    MicroLRU::Cache<action_filter, std::vector<t_action *>> filter_result_cache{256,
                                                                                [](const std::vector<t_action *> &) {}};
};

static t_action_manager g_mgr{};

/**
 * \brief Finds all actions using the given filter.
 */
static std::vector<t_action *> get_action_ptrs_matching_filter(const action_filter &filter)
{
    if (g_mgr.filter_result_cache.contains(filter))
    {
        return g_mgr.filter_result_cache.get(filter).value();
    }

    const auto normalized_filter = ActionManager::normalize_filter(filter);
    std::vector<t_action *> result;

    // Special case: pure wildcard filter, matches everything.
    if (normalized_filter == L"*")
    {
        result.reserve(g_mgr.actions.size());
        for (auto &action : g_mgr.actions)
        {
            result.emplace_back(&action);
        }

        g_mgr.filter_result_cache.add(filter, result);
        return result;
    }

    const auto filter_segments = ActionManager::get_segments(normalized_filter);
    if (filter_segments.empty())
    {
        g_mgr.filter_result_cache.add(filter, result);
        return result;
    }

    const bool has_wildcard = filter_segments.back() == L"*";
    const size_t filter_segments_to_compare = has_wildcard ? filter_segments.size() - 1 : filter_segments.size();

    for (auto &action : g_mgr.actions)
    {
        if (has_wildcard)
        {
            // The path must have more segments than the filter if the filter ends with a wildcard, otherwise we aren't
            // deep enough.
            if (action.segments.size() <= filter_segments_to_compare)
            {
                continue;
            }
        }
        else
        {
            if (action.segments.size() != filter_segments_to_compare)
            {
                continue;
            }
        }

        bool is_match = true;
        for (size_t i = 0; i < filter_segments_to_compare; ++i)
        {
            if (action.segments[i] != filter_segments[i])
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

    g_mgr.filter_result_cache.add(filter, result);
    return result;
}

/**
 * \brief Tries to resolve a fully-qualified action path to a single action pointer.
 */
static t_action *get_single_action_ptr_matching_path(const action_path &path)
{
    if (path.contains(L"*"))
    {
        g_view_logger->error(L"ActionManager::get_single_action_ptr_matching_filter: Expected path without wildcard.");
        return nullptr;
    }

    const auto actions = get_action_ptrs_matching_filter(path);

    if (actions.empty())
    {
        return nullptr;
    }

    if (actions.size() > 1)
    {
        return nullptr;
    }

    return actions.front();
}

/**
 * \brief Checks whether the given fully-qualified action path is valid.
 */
static bool validate_action_path(const std::wstring &path)
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
 * \brief Creates of action paths from one of action pointers.
 */
static std::vector<std::wstring> map_action_ptrs_to_paths(const std::vector<t_action *> &actions)
{
    std::vector<std::wstring> paths;
    paths.reserve(actions.size());

    for (const auto &action : actions)
    {
        paths.emplace_back(action->params.path);
    }

    return paths;
}

/**
 * \brief Updates the display names of the specified actions and returns the actions mapped to action paths.
 */
static std::vector<std::wstring> update_display_names(const std::vector<t_action *> &actions)
{
    for (auto &action : actions)
    {
        const auto &name = action->segments.back();
        const bool has_separator = name.ends_with(ActionManager::SEPARATOR_SUFFIX);

        std::wstring display_name;

        if (has_separator)
        {
            display_name = name.substr(0, name.size() - ActionManager::SEPARATOR_SUFFIX.size());
        }
        else
        {
            display_name = name;
        }

        action->raw_name = display_name;

        if (action->params.get_display_name)
        {
            const auto override_display_name = action->params.get_display_name();
            if (!override_display_name.empty())
            {
                display_name = override_display_name;
            }
        }

        action->display_name = std::make_optional(display_name);
    }
    return map_action_ptrs_to_paths(actions);
}

/**
 * \brief Updates the enabled states of the specified actions and returns the actions mapped to action paths.
 */
static std::vector<std::wstring> update_enabled_states(const std::vector<t_action *> &actions)
{
    for (auto &action : actions)
    {
        action->enabled = std::make_optional(action->params.get_enabled ? action->params.get_enabled() : true);
    }
    return map_action_ptrs_to_paths(actions);
}

/**
 * \brief Notifies about the active state of actions changing.
 */
static std::vector<std::wstring> update_active_states(const std::vector<t_action *> &actions)
{
    for (auto &action : actions)
    {
        action->active = std::make_optional(action->params.get_active ? action->params.get_active() : false);
    }
    return map_action_ptrs_to_paths(actions);
}

/**
 * \brief Notifies about the action registry changing.
 */
static void notify_action_registry_changed()
{
    Messenger::broadcast(Messenger::Message::ActionRegistryChanged, nullptr);
}

bool ActionManager::add(const t_action_params &params)
{
    const auto normalized_path = normalize_filter(params.path);

    if (!validate_action_path(normalized_path))
    {
        g_view_logger->error(L"ActionManager::add: Malformed action path '{}'.", normalized_path);
        return false;
    }

    // > If an action with the same path already exists, the operation will fail.
    if (get_single_action_ptr_matching_path(normalized_path) != nullptr)
    {
        g_view_logger->error(L"ActionManager::add: Action with path '{}' already exists.", normalized_path);
        return false;
    }

    // > If adding the action causes another action to gain a child (e.g. there's an action `A > B`, and we're adding `A
    // > B > C > D`), the operation will fail.
    const auto segments = get_segments(normalized_path);

    // 1. Look for an action at each segment
    for (size_t i = 0; i < segments.size(); ++i)
    {
        // a. Build the path for each segment
        std::wstring segment_slice;
        for (size_t j = 0; j <= i; ++j)
        {
            if (j > 0)
            {
                segment_slice += SEGMENT_SEPARATOR;
            }
            segment_slice += segments[j];
        }

        // b. Check if this potential parent exists
        if (get_single_action_ptr_matching_path(segment_slice) != nullptr)
        {
            g_view_logger->error(
                L"ActionManager::add: Adding '{}' would make '{}' gain a direct child, which is not allowed.",
                normalized_path, segment_slice);
            return false;
        }
    }

    t_action action{};
    action.params = params;
    action.params.path = normalized_path;
    action.segments = segments;

    g_mgr.actions.emplace_back(action);
    g_mgr.filter_result_cache.clear();

    if (!g_config.hotkeys.contains(normalized_path) || !g_config.inital_hotkeys.contains(normalized_path))
    {
        g_config.hotkeys[normalized_path] = Hotkey::t_hotkey::make_unassigned();
        g_config.inital_hotkeys[normalized_path] = Hotkey::t_hotkey::make_unassigned();
    }

    if (!g_mgr.batched_work)
    {
        notify_action_registry_changed();
    }

    return true;
}

std::vector<action_path> ActionManager::remove(const action_filter &filter)
{
    const auto actions = get_action_ptrs_matching_filter(filter);

    std::vector<action_path> removed_paths;
    removed_paths.reserve(actions.size());

    // Call the on_removed callbacks first and before removing anything - we don't want weirdness if the callbacks do
    // some bullshit like calling back into the ActionManager...
    for (const auto &action_to_be_removed : actions)
    {
        for (const auto &existing_action : g_mgr.actions)
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

    for (const auto &action_to_be_removed : actions)
    {
        std::erase_if(g_mgr.actions,
                      [&](const t_action &a) { return a.params.path == action_to_be_removed->params.path; });
    }

    g_mgr.filter_result_cache.clear();

    if (!g_mgr.batched_work)
    {
        Messenger::broadcast(Messenger::Message::ActionRegistryChanged, nullptr);
    }

    return removed_paths;
}

bool ActionManager::associate_hotkey(const action_path &path, const Hotkey::t_hotkey &hotkey, bool overwrite_existing)
{
    t_action *action = get_single_action_ptr_matching_path(path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::associate_hotkey: '{}' didn't resolve to an action", path);
        return false;
    }

    const auto normalized_path = action->params.path;

    RT_ASSERT(g_config.hotkeys.contains(normalized_path) && g_config.inital_hotkeys.contains(normalized_path),
              L"Action didn't have a hotkey entry.");

    const bool has_assignment = g_config.hotkeys.at(normalized_path).is_assigned();

    if (overwrite_existing)
    {
        if (!has_assignment)
        {
            g_config.inital_hotkeys[normalized_path] = hotkey;
        }

        g_config.hotkeys[normalized_path] = hotkey;
    }
    else
    {
        if (!has_assignment)
        {
            g_config.hotkeys[normalized_path] = hotkey;
            g_config.inital_hotkeys[normalized_path] = hotkey;
        }
    }

    if (!g_mgr.batched_work)
    {
        notify_action_registry_changed();
    }

    return true;
}

std::wstring ActionManager::get_display_name(const action_filter &filter, bool ignore_override)
{
    const auto actions = get_action_ptrs_matching_filter(filter);

    if (actions.empty() || actions.size() > 1)
    {
        // It's a filter, not a fully-qualified action path. We don't look up anything, but just do some formatting
        // instead.
        auto name = get_segments(filter).back();
        if (name.ends_with(SEPARATOR_SUFFIX))
        {
            name = MiscHelpers::trim(name.substr(0, name.size() - SEPARATOR_SUFFIX.size()));
        }
        return name;
    }

    const auto action = actions.front();

    if (ignore_override)
    {
        return action->raw_name;
    }

    if (!action->display_name.has_value())
    {
        update_display_names({action});
    }

    return action->display_name.value();
}

bool ActionManager::get_enabled(const action_path &path)
{
    t_action *action = get_single_action_ptr_matching_path(path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::get_action_enabled: '{}' didn't resolve to an action", path);
        return false;
    }

    if (!action->enabled.has_value())
    {
        update_enabled_states({action});
    }

    return action->enabled.value();
}

bool ActionManager::get_active(const action_path &path)
{
    t_action *action = get_single_action_ptr_matching_path(path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::get_action_active: '{}' didn't resolve to an action", path);
        return false;
    }

    if (!action->active.has_value())
    {
        update_active_states({action});
    }

    return action->active.value();
}

bool ActionManager::get_activatability(const action_path &path)
{
    t_action *action = get_single_action_ptr_matching_path(path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::get_action_activatability: '{}' didn't resolve to an action", path);
        return false;
    }

    return action->params.get_active != nullptr;
}

void ActionManager::begin_batch_work()
{
    g_mgr.batched_work = true;
}

void ActionManager::end_batch_work()
{
    g_mgr.batched_work = false;
    notify_action_registry_changed();
}

void ActionManager::notify_display_name_changed(const action_filter &filter)
{
    const auto updated_actions = update_display_names(get_action_ptrs_matching_filter(filter));
    Messenger::broadcast(Messenger::Message::ActionDisplayNameChanged, updated_actions);
}

void ActionManager::notify_enabled_changed(const action_filter &filter)
{
    const auto updated_actions = update_enabled_states(get_action_ptrs_matching_filter(filter));
    Messenger::broadcast(Messenger::Message::ActionEnabledChanged, updated_actions);
}

void ActionManager::notify_active_changed(const action_filter &filter)
{
    const auto updated_actions = update_active_states(get_action_ptrs_matching_filter(filter));
    Messenger::broadcast(Messenger::Message::ActionActiveChanged, updated_actions);
}

std::vector<action_path> ActionManager::get_actions_matching_filter(const action_filter &filter)
{
    const auto actions = get_action_ptrs_matching_filter(filter);

    std::vector<action_path> result;
    result.reserve(actions.size());

    for (const auto &action : actions)
    {
        result.emplace_back(action->params.path);
    }

    return result;
}

std::vector<action_filter> ActionManager::get_segments(const action_filter &filter)
{
    if (g_mgr.segment_cache.contains(filter))
    {
        return g_mgr.segment_cache.get(filter).value();
    }

    // std::vector<action_filter> parts = StrUtils::split_wstring(filter, SEGMENT_SEPARATOR);
    // for (auto &part : parts)
    // {
    //     part = MiscHelpers::trim(part);
    // }

    // std::erase_if(parts, [](const std::wstring &part) { return part.empty(); });

    auto parts = StrUtils::split_wstring(filter, SEGMENT_SEPARATOR) |
                 std::views::transform([](std::wstring_view part) { return StrUtils::ctrim_wstring(part); }) |
                 std::views::filter([](std::wstring_view part) { return !part.empty(); }) |
                 std::views::transform([](std::wstring_view part) { return std::wstring(part); }) |
                 std::ranges::to<std::vector<action_filter>>();

    g_mgr.segment_cache.add(filter, parts);

    return parts;
}

ActionManager::action_filter ActionManager::normalize_filter(const action_filter &filter)
{
    const auto parts = get_segments(filter);
    return MiscHelpers::join_wstring(parts, SEGMENT_SEPARATOR);
}

void ActionManager::invoke(const action_path &path, const bool up, const bool release_on_repress)
{
    t_action *action = get_single_action_ptr_matching_path(path);

    if (!action)
    {
        g_view_logger->error(L"ActionManager::invoke: '{}' didn't resolve to an action", path);
        return;
    }

    if (action->params.get_enabled && !action->params.get_enabled())
    {
        return;
    }

    if (up)
    {
        action->pressed = false;

        if (action->params.on_release)
        {
            action->params.on_release();
        }
    }
    else
    {
        if (release_on_repress && action->params.on_release && action->pressed)
        {
            action->params.on_release();
            action->pressed = false;
            return;
        }

        if (action->params.on_press)
        {
            action->params.on_press();
        }

        action->pressed = true;
    }
}

void ActionManager::lock_hotkeys(const bool lock)
{
    g_mgr.lock_hotkeys = lock;
}

bool ActionManager::get_hotkeys_locked()
{
    return g_mgr.lock_hotkeys;
}
