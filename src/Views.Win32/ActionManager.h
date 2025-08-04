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
     * \brief A fully-qualified action path. Must be in the format <c>"Category > Subcategory[] > Name"</c>. There can be an arbitrary number of subcategories.
     */
    using fq_action_path = std::wstring;

    /**
     * \brief A partially-qualified action path. Must be in the same format as <c>fq_action_path</c>, but can be missing the name segment.
     * This is used to refer to actions without specifying the full path, such as when notifying about changes.
     */
    using pq_action_path = std::wstring;

    /**
     * \brief Represents action creation parameters.
     */
    struct t_action_params {
        /**
         * \brief The action's path.
         */
        fq_action_path path{};

        /**
         * \brief The callback to be invoked when the action is initially triggered.
         */
        std::function<void()> down_callback = [] {
        };

        /**
         * \brief The callback to be invoked when the action has been released. Can be null.
         */
        std::function<void()> up_callback = [] {
        };

        /**
         * \brief The function used to determine whether the action is enabled.
         */
        std::function<bool()> get_enabled = [] {
            return true;
        };

        /**
         * \brief The function used to determine whether the action is "active". The active state means a checked state in the menu.
         */
        std::function<bool()> get_active = [] {
            return false;
        };

        /**
         * \brief The function used to determine the function's real name, which is an override for the path-derived display name.
         * If this function is null or returns an empty string, the action's display name will be derived from its path.
         */
        std::function<std::wstring()> get_real_name = [] {
            return L"";
        };
    };

    struct t_action {
        t_action_params params{};

        [[nodiscard]] std::wstring display_name(bool ignore_real_name = false) const;
    };

    /**
     * \brief Adds the specified action to the action registry, removing any existing action with the same path.
     * \param params The action parameters.
     * \remarks Whether the operation succeeded.
     */
    bool add(const t_action_params& params);

    /**
     * \brief Associates a hotkey with an action by its path, while replacing any existing hotkey association for that action.
     * \param path The action path.
     * \param hotkey The hotkey to associate with the action.
     * \return Whether the operation succeeded.
     * \details This updates the action<->hotkey associations in the config.
     * \details If this is the first time the hotkey is associated with the action.
     */
    bool associate_hotkey(const fq_action_path& path, const Hotkey::t_hotkey& hotkey);

    /**
     * \brief Begins a batch operation. Batches all updates caused by <c>add</c> and <c>associate_hotkey</c> into one at the end of the operation.
     */
    void begin_batch_work();

    /**
     * \brief Ends a batch operation.
     */
    void end_batch_work();

    /**
     * \brief Notifies about the enabled state of an action or a group of actions changing.
     * \param path The action path. If the path is unqualified, all actions under the last category or subcategory will be considered changed.
     */
    void notify_enabled_changed(const pq_action_path& path);

    /**
     * \brief Notifies about the active state of an action or a group of actions changing.
     * \param path The action path. If the path is unqualified, all actions under the last category or subcategory will be considered changed.
     */
    void notify_active_changed(const pq_action_path& path);

    /**
     * \brief Notifies about the real name of an action or a group of actions changing.
     * \param path The action path. If the path is unqualified, all actions under the last category or subcategory will be considered changed.
     */
    void notify_real_name_changed(const pq_action_path& path);

    /**
     * \brief Gets the display name for an action.
     * \param path The action path.
     * \param ignore_real_name Whether to ignore the real name override.
     * \return The action's display name or an empty string if the display name couldn't be resolved.
     */
    std::wstring get_display_name(const pq_action_path& path, bool ignore_real_name = false);

    /**
     * \brief Gets a copy of all registered actions.
     * \param path The action path filter. If the path is unqualified, all actions under the last category or subcategory will be returned. If the path is empty, all actions will be returned.
     */
    std::vector<t_action> get_actions(const pq_action_path& path = L"");

    /**
     * \brief Gets the segments of an action's path.
     * \param path The qualified or partially-qualified path to split.
     * \return A vector of segments, where each segment is a part of the path.
     */
    std::vector<std::wstring> get_path_segments(const pq_action_path& path);

    /**
     * \brief Manually invokes an action by its path.
     * \param path The qualified path of the action to invoke.
     * \param down Whether the invocation is a down (press) or up (release) action.
     */
    void invoke(const std::wstring& path, bool down = true);
} // namespace ActionManager
