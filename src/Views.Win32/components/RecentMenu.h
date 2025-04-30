/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief Implementation of a recent menu functionality provider.
 */
namespace RecentMenu
{
    /**
     * \brief Clears the menu and rebuilds it with the provided vector of strings.
     * \param vec The vector of strings to populate the menu with.
     * \param first_menu_id The menu ID for the first element.
     * \param parent_menu The handle to the parent menu.
     */
    void build(const std::vector<std::wstring>& vec, int first_menu_id, HMENU parent_menu);

    /**
     * \brief Adds a new element to the recent menu.
     * \param vec The vector of strings to add the element to.
     * \param val The value to add.
     * \param frozen Whether the list is frozen.
     * \param first_menu_id The menu ID for the first element.
     * \param parent_menu The handle to the parent menu.
     */
    void add(std::vector<std::wstring>& vec, std::wstring val, bool frozen, int first_menu_id, HMENU parent_menu);

    /**
     * \brief Gets the element at the specified index in the menu.
     * \param vec The vector of strings to get the element from.
     * \param first_menu_id The menu ID for the first element.
     * \param menu_id The menu ID of the element to get.
     * \return The element at the specified index. Can be empty.
     */
    std::wstring element_at(std::vector<std::wstring> vec, int first_menu_id, int menu_id);

} // namespace RecentMenu
