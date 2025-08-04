/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <ActionManager.h>

/**
 * \brief Implementation of a recent menu functionality provider.
 */
namespace RecentMenu
{
    /**
     * \brief Adds a new element to a recent item vector.
     * \param recent_menu_path The path to the relevant recent menu.
     * \param vec The vector of recent items to add the element to.
     * \param val The value to add.
     * \param frozen Whether the list is frozen.
     */
    void add(const ActionManager::pq_action_path& recent_menu_path, std::vector<std::wstring>& vec, std::wstring val, bool frozen);
} // namespace RecentMenu
