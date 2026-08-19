/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common.Views/ActionManager.hpp>

/**
 * \brief A module responsible for managing recent items.
 */
namespace RecentMenu
{
/**
 * \brief The maximum number of recent items to keep in the menu.
 */
const size_t MAX_RECENT_ITEMS = 5;

/**
 * \brief Adds a new recent item to the specified recent item vector.
 * \param menu_path The path to the relevant recent menu.
 * \param vec The vector of recent items.
 * \param val The value to add.
 * \param frozen Whether the new item shouldn't be added to the list.
 */
void add(const ActionManager::action_filter &menu_path, std::vector<std::string> &vec, std::string val, bool frozen);
} // namespace RecentMenu
