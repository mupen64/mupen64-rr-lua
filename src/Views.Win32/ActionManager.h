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
    const std::wstring SEPARATOR_SUFFIX = L" ---";

    /**
     * \brief Represents action creation parameters.
     */
    struct t_action_params {
        /**
         * \brief The action's qualified path, consisting of a category, subcategories, and an action name.
         * \details Must be in the format <c>"Category > Subcategory[] > Name"</c>. There can be an arbitrary number of subcategories.
         */
        std::wstring path{};

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

        [[nodiscard]] std::wstring display_name() const;
    };

    /**
     * \brief Adds the specified action to the action registry, removing any existing action with the same path.
     * \param params The action parameters.
     * \remarks Whether the operation succeeded.
     */
    bool add(const t_action_params& params);

    /**
     * \brief Associates a hotkey with an action by its path, while replacing any existing hotkey association for that action.
     * \param path The qualified path of the action to associate the hotkey with, consisting of a category, subcategories, and an action name. Must be in the format <c>"Category > Subcategory[] > Name"</c>. There can be an arbitrary number of subcategories.
     * \param hotkey The hotkey to associate with the action.
     * \return Whether the operation succeeded.
     * \details This updates the action<->hotkey associations in the config.
     * \details If this is the first time the hotkey is associated with the action.
     */
    bool associate_hotkey(const std::wstring& path, const Hotkey::t_hotkey& hotkey);

    /**
     * \brief Begins a batch operation. Batches all updates caused by <c>add</c> and <c>associate_hotkey</c> into one at the end of the operation.
     */
    void begin_batch_work();

    /**
     * \brief Ends a batch operation.
     */
    void end_batch_work();

    /**
     * \brief Notifies the ActionManager that the enabled state of an action has changed.
     * \param path The qualified path of the action whose enabled state has changed. If the path doesn't end with an action's name, all actions under that category or subcategory will be considered changed.
     */
    void notify_enabled_changed(const std::wstring& path);

    /**
     * \brief Notifies the ActionManager that the active state of an action has changed.
     * \param path The qualified path of the action whose active state has changed. If the path doesn't end with an action's name, all actions under that category or subcategory will be considered changed.
     */
    void notify_active_changed(const std::wstring& path);

    /**
     * \brief Notifies the ActionManager that the real name of an action has changed.
     * \param path The qualified path of the action whose real name has changed. If the path doesn't end with an action's name, all actions under that category or subcategory will be considered changed.
     */
    void notify_real_name_changed(const std::wstring& path);

    /**
     * \brief Gets the display name for an action or part of an action.
     * \param path The qualified or unqualified path of the action to get the display name for. Can be missing arbitrary segments.
     * \return The action's display name or an empty string if the display name couldn't be resolved.
     */
    std::wstring get_display_name(const std::wstring& path);

    /**
     * \brief Gets a copy of all registered actions.
     */
    std::vector<t_action> get_actions();

    /**
     * \brief Gets the segments of an action's path.
     * \param path The fully-qualified path of the action to get the segments for.
     * \return A vector of segments, where each segment is a part of the path.
     */
    std::vector<std::wstring> get_path_segments(const std::wstring& path);
    
    /**
     * \brief Manually invokes an action by its path.
     * \param path The qualified path of the action to invoke.
     */
    void invoke(const std::wstring& path);
} // namespace ActionManager
