/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for managing actions.
 */
namespace ActionManager
{
    /**
     * \brief Represents a combination of key + modifier combination.
     */
    struct t_hotkey {
        int32_t key{};
        bool ctrl{};
        bool shift{};
        bool alt{};
        [[nodiscard]] bool is_nothing() const;
        [[nodiscard]] std::wstring to_wstring() const;
    };

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
     * \brief Handles interactions with a menu item. The interaction will only be handled if the menu was built by the ActionManager.
     * \param id The menu item's ID.
     * \return Whether the interaction was handled.
     */
    bool handle_menu_interaction(size_t id);
} // namespace ActionManager
