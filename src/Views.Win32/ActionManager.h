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
        [[nodiscard]] std::wstring to_wstring() const;
    };

    /**
     * \brief Represents an action.
     */
    struct t_action {
        /**
         * \brief The action's name.
         * \details Must be in the format "Category > Subcategory[] > Name". There can be an arbitrary number of subcategories.
         */
        std::wstring name{};

        /**
         * \brief The hotkey associated with the action.
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

    /**
     * \brief Represents a command associated with an action as part of a tree structure.
     */
    struct t_command_node {
        std::wstring name{};
        t_action* action{};
        uint16_t menu_id{};
        std::vector<t_command_node> children{};
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
     * \brief Handles interactions with a menu item. The interaction will only be handled if the menu was built by the ActionManager.
     * \param id The menu item's ID.
     * \return Whether the interaction was handled.
     */
    bool handle_menu_interaction(size_t id);
} // namespace ActionManager
