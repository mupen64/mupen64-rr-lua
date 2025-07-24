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
     * \brief Represents an action.
     */
    struct t_action {
        /**
         * \brief The action's qualified path, consisting of a category, subcategories, and an action name.
         * \details Must be in the format <c>"Category > Subcategory[] > Name"</c>. There can be an arbitrary number of subcategories.
         */
        std::wstring path{};

        /**
         * \brief The hotkey associated with the action. Is considered "nothing" if the key is 0 and all modifiers are false.
         */
        t_hotkey hotkey{};

        /**
         * \brief The callback to be invoked when the action is initially triggered.
         */
        std::function<void()> down_callback{};

        /**
         * \brief The callback to be invoked when the action has been released. Can be null.
         */
        std::function<void()> up_callback{};
    };


    // TODO: Add functionality for adding separators by specifying an action name of "---".

    /**
     * \brief Adds the specified action to the action registry, removing any existing action with the same path.
     * \param path The action's qualified path, consisting of a category, subcategories, and an action name. Must be in the format <c>"Category > Subcategory[] > Name"</c>. There can be an arbitrary number of subcategories.
     * \param down_callback The callback to be invoked when the action is initially triggered.
     * \param up_callback The callback to be invoked when the action has been released. Can be null.
     * \remarks Whether the operation succeeded.
     */
    bool add(const std::wstring& path, const std::function<void()>& down_callback, const std::function<void()>& up_callback = {});

    /**
     * \brief Associates a hotkey with an action by its path.
     * \param path The qualified path of the action to associate the hotkey with, consisting of a category, subcategories, and an action name. Must be in the format <c>"Category > Subcategory[] > Name"</c>. There can be an arbitrary number of subcategories.
     * \param hotkey The hotkey to associate with the action.
     * \return Whether the operation succeeded.
     */
    bool associate_hotkey(const std::wstring& path, const t_hotkey& hotkey);

    /**
     * \brief Performs the combined action of adding an action and associating a hotkey with it.
     * \return Whether the operation succeeded.
     * \remarks Will unregister the action if hotkey association fails. See <c>add</c> and <c>associate_hotkey</c> for more details.
     */
    bool add_and_associate_hotkey(const std::wstring& path, const t_hotkey& hotkey, const std::function<void()>& down_callback, const std::function<void()>& up_callback = {});

    /**
     * \brief Handles interactions with a menu item. The interaction will only be handled if the menu was built by the ActionManager.
     * \param id The menu item's ID.
     * \return Whether the interaction was handled.
     */
    bool handle_menu_interaction(size_t id);

    /**
     * \brief Manually invokes an action by its path.
     * \param path The qualified path of the action to invoke.
     */
    void invoke(const std::wstring& path);

    /**
     * \brief Gets a copy of all currently registered actions.
     */
    std::vector<t_action> get_actions();
} // namespace ActionManager
