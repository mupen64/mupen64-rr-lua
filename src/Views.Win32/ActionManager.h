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
     * \brief Represents an action.
     */
    struct t_action {
        struct t_hotkey {
            int32_t key{};
            bool ctrl{};
            bool shift{};
            bool alt{};
            std::wstring to_wstring() const;
        };

        /**
         * \brief The name of the action. Must be unique.
         */
        std::wstring name{};

        /**
         * \brief The action's unique identifier. Automatically assigned during registration.
         */
        size_t id{};

        /**
         * \brief The hotkey associated with the action.
         */
        t_hotkey hotkey{};

        /**
         * \brief The callback to be invoked when the action is triggered.
         */
        std::function<void()> down_callback{};
        std::function<void()> up_callback{};
    };

    /**
     * \brief Adds the specified action to the action registry, removing any existing action with the same name.
     * \param action The action to add.
     */
    void add(t_action action);

    /**
     * \brief Gets a reference to an action by its name.
     * \param name The name of the action to retrieve.
     * \return A reference to the action if found, otherwise std::nullopt.
     */
    std::optional<std::reference_wrapper<t_action>> get_by_name(const std::wstring& name);

    /**
     * \brief Gets the actions in the action registry.
     */
    std::vector<t_action> get_actions();

    /**
     * \brief Handles interactions with a menu item.
     * \param id The ID of the interacted menu item.
     * \return Whether the menu item was handled.
     */
    bool handle_menu_interaction(int id);

    /**
     * \brief Builds the application menu based on the currently registered actions.
     */
    void build_menu();

} // namespace ActionManager
