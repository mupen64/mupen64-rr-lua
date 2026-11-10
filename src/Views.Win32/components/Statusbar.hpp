/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing statusbar functionality.
 */
namespace Statusbar
{
/**
 * \brief Creates the statusbar.
 */
void create();

/**
 * \brief Gets the statusbar handle
 */
HWND hwnd();

} // namespace Statusbar
