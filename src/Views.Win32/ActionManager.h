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
     * \brief The suffix for action path segments that are used to indicate a separator.
     */
    const std::wstring SEPARATOR_SUFFIX = L" ---";

    /**
     * \brief An action filter that can be either a fully-qualified or partially-qualified `"Category > Subcategory[] [ > Name ]"`.
     * This is usually used to refer to groups of actions, but can also refer to a single action.
     */
    using action_filter = std::wstring;

    /**
     * \brief A fully-qualified action path in the format `"Category > Subcategory[] > Name"`.
     * An action path is a subset of the action filter that is guaranteed to be fully-qualified, meaning it contains all segments of the path.
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
         * \brief The callback to be invoked when the action is initially triggered.
         */
        std::function<void()> down_callback = [] {
        };

        /**
         * \brief The callback to be invoked when the action has been released. Can be null.
         */
        std::function<void()> up_callback;

        /**
         * \brief The callback to be invoked prior to the action being removed from the registry. Can be null.
         */
        std::function<void()> on_removed;

        /**
         * \brief The function used to determine whether the action is enabled. If null, the action will be considered enabled.
         */
        std::function<bool()> get_enabled;

        /**
         * \brief The function used to determine whether the action is "active". The active state usually means a checked or toggled UI state. If null, the action will be considered inactive.
         */
        std::function<bool()> get_active;

        /**
         * \brief The function used to determine the function's display name. If null, the display name will be derived from the path.
         */
        std::function<std::wstring()> get_display_name;
    };

    /**
     * \brief Adds an action to the action registry. Any action with the same path will be replaced.
     * \param params The action parameters.
     * \return Whether the operation succeeded.
     */
    bool add(const t_action_params& params);

    /**
     * \brief Removes actions matching the specified filter.
     * \param filter A filter.
     * \return A vector containing the paths of the actions that were removed.
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
     * \brief Begins a batch operation. Batches all updates caused by <c>add</c>, <c>remove</c>, and <c>associate_hotkey</c> into one at the succeeding call to <c>end_batch_work</c>.
     */
    void begin_batch_work();

    /**
     * \brief Ends a batch operation.
     */
    void end_batch_work();

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
     * \brief Notifies about the display name of actions matching a filter changing.
     * \param filter A filter.
     */
    void notify_display_name_changed(const action_filter& filter);

    /**
     * \brief Gets the display name for a given filter.
     * \param filter A filter.
     * \param ignore_override Whether to ignore the display name override.
     * \return The action's display name or an empty string if the display name couldn't be resolved.
     */
    std::wstring get_display_name(const action_filter& filter, bool ignore_override = false);

    /**
     * \brief Gets all action paths that match the specified filter.
     * \param filter The action path filter. If the path is unqualified, all actions under the last category or subcategory will be returned. If the path is empty, all actions will be returned.
     */
    std::vector<action_path> get_actions_matching_filter(const action_filter& filter = L"");

    /**
     * \brief Gets the segments of a filter.
     * \param filter A filter.
     * \return A vector of the filter's segments.
     */
    std::vector<action_path> get_segments(const action_filter& filter);

    /**
     * \brief Gets whether an action is enabled.
     * \param path A path.
     * \return The actions' enabled state.
     */
    bool get_action_enabled(const action_path& path);

    /**
     * \brief Gets whether an action is active.
     * \param path A path.
     * \return The actions' active state.
     */
    bool get_action_active(const action_path& path);

    /**
     * \brief Manually invokes an action by its path.
     * \param path A path.
     * \param up Whether the invocation is considered as "releasing" the action.
     */
    void invoke(const action_path& path, bool up = false);
} // namespace ActionManager
