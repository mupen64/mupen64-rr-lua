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
    // TODO: "Menu mixin" support, which allows arbitrary menu items to be added after a specific action.
    // TODO: Checkable "action" menu item support... Oof...

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
} // namespace ActionManager
