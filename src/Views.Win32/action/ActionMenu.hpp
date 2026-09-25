/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common.Views/ActionManager.hpp>

/**
 * \brief A module responsible for providing menu functionality surrounding menus filled with data from the
 * ActionManager.
 */
namespace ActionMenu
{
/**
 * \brief Initializes the ActionMenu module.
 */
void init();

/**
 * \brief Adds a managed menu to the specified window.
 * \param hwnd Handle to a top-level window.
 * \param include_filter Optional filter for including actions from the menu. If not specified, all actions are included.
 * @param exclude_filter Optional filter for excluding actions from the menu. If not specified, no actions are excluded.
 * @param root Optional action filter whose path segments are removed from the displayed menu hierarchy. Actions below the
 * root are displayed directly beneath the menu bar.
 */
bool add_managed_menu(HWND hwnd, std::optional<ActionManager::action_path> include_filter = std::nullopt, std::optional<ActionManager::action_path> exclude_filter = std::nullopt, std::optional<ActionManager::action_filter> root = std::nullopt);
} // namespace ActionMenu
