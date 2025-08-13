/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Hotkey.h>

/**
 * \brief A module responsible for managing actions.
 */
namespace ActionManager
{
    /**
     * \brief The character used to separate segments in action paths and filters.
     */
    const std::wstring SEGMENT_SEPARATOR = L">";

    /**
     * \brief The suffix for action path and filters segments that are used to indicate a separator.
     */
    const std::wstring SEPARATOR_SUFFIX = L"---";

    /**
     * \brief An action filter that can be used to match actions in the action registry.
     * Can be in the format `[Category[] | *] > [Name | *]`.
     * The `*` wildcard can be used to match any child from that segment onwards.
     * The wildcard must always be the last segment in the filter: wildcard-based wide lookups like `A > * > C` aren't supported.
     *
     * Example queries:
     * `*` - matches all actions.
     * `Mupen64 > File > *` - matches "Mupen64 > File > Load ROM...", "Mupen64 > File > Recent ROMs > Load Recent Item #5", etc...
     * `Mupen64 > File` - matches nothing, because `File` has no action associated with it.
     */
    using action_filter = std::wstring;

    /**
     * \brief A fully-qualified action path in the format `"Category[] > Name"`.
     * An action path is a subset of the action filter that contains no wildcards and is used to uniquely identify an action.
     */
    using action_path = std::wstring;

    /**
     * \brief Represents action creation parameters.
     */
    struct t_action_params {
        /**
         * \brief The action's path.
         */
        action_path path{};

        /**
         * \brief The callback to be invoked when the action is pressed. Can be null.
         */
        std::function<void()> on_press;

        /**
         * \brief The callback to be invoked when the action is released. Can be null.
         */
        std::function<void()> on_release;

        /**
         * \brief The callback to be invoked prior to the action being removed from the registry. Can be null.
         */
        std::function<void()> on_removed;

        /**
         * \brief The function used to determine the function's display name. If null, the display name will be derived from the path.
         */
        std::function<std::wstring()> get_display_name;

        /**
         * \brief The function used to determine whether the action is enabled. If null, the action will be considered enabled.
         */
        std::function<bool()> get_enabled;

        /**
         * \brief The function used to determine whether the action is "active". The active state usually means a checked or toggled UI state. If null, the action will be considered inactive.
         */
        std::function<bool()> get_active;
    };

    /**
     * \brief Adds an action to the action registry.
     * If an action with the same path already exists, the operation will fail.
     * If adding the action causes another action to gain a child (e.g. there's an action `A > B`, and we're adding `A > B > C > D`), the operation will fail. To add the action, delete the original action (`A > B`) first.
     * \param params The action parameters.
     * \return Whether the operation succeeded.
     */
    bool add(const t_action_params& params);

    /**
     * \brief Removes actions matching the specified filter.
     * \param filter A filter.
     * \return A collection containing the paths of the actions that were removed.
     */
    std::vector<action_path> remove(const action_filter& filter);

    /**
     * \brief Associates a hotkey with an action by its path, while replacing any existing hotkey association for that action.
     * \param path A path.
     * \param hotkey The hotkey to associate with the action.
     * \param overwrite_existing Whether the any existing hotkey association will be overwritten. If false, the hotkey will only be associated if the action has no hotkey associated with it already.
     * \return Whether the operation succeeded.
     */
    bool associate_hotkey(const action_path& path, const Hotkey::t_hotkey& hotkey, bool overwrite_existing = true);

    /**
     * \brief Begins a batch operation. Batches all updates caused by `add`, `remove`, and `associate_hotkey` into one at the succeeding call to `end_batch_work`.
     */
    void begin_batch_work();

    /**
     * \brief Ends a batch operation.
     */
    void end_batch_work();

    /**
     * \brief Notifies about the display name of actions matching a filter changing.
     * \param filter A filter.
     */
    void notify_display_name_changed(const action_filter& filter);

    /**
     * \brief Notifies about the enabled state of actions matching a filter changing.
     * \param filter A filter.
     */
    void notify_enabled_changed(const action_filter& filter);

    /**
     * \brief Notifies about the active state of actions matching a filter changing.
     * \param filter A filter.
     */
    void notify_active_changed(const action_filter& filter);

    /**
     * \brief Gets the display name for a given filter.
     * \param filter A filter.
     * \param ignore_override Whether to ignore the display name override.
     * \return The action's display name or an empty string if the display name couldn't be resolved.
     */
    std::wstring get_display_name(const action_filter& filter, bool ignore_override = false);

    /**
     * \brief Gets whether an action is enabled.
     * \param path A path.
     * \return The actions' enabled state.
     */
    bool get_enabled(const action_path& path);

    /**
     * \brief Gets whether an action is active.
     * \param path A path.
     * \return The actions' active state.
     */
    bool get_active(const action_path& path);

    /**
     * \brief Gets all action paths that match the specified filter.
     * \param filter A filter.
     * \return A collection of action paths that match the filter.
     */
    std::vector<action_path> get_actions_matching_filter(const action_filter& filter);

    /**
     * \brief Gets the segments of a filter.
     * \param filter A filter.
     * \return A collection of the filter's segments.
     */
    std::vector<action_filter> get_segments(const action_filter& filter);

    /**
     * \brief Normalizes a filter by splitting it into segments and joining them back together using the segment separator.
     * \param filter A filter.
     * \return The normalized filter.
     */
    action_filter normalize_filter(const action_filter& filter);

    /**
     * \brief Manually invokes an action by its path. If the action has an up callback, is already pressed down, and `up` is false, only the up callback will be invoked.
     * \param path A path.
     * \param up If true, the action is considered to be released, otherwise it is considered to be pressed down.
     */
    void invoke(const action_path& path, bool up = false);
} // namespace ActionManager
