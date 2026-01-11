/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

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
 */
bool add_managed_menu(HWND hwnd);
} // namespace ActionMenu
