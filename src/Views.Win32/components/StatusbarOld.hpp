/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief The legacy statusbar.
 * \remarks Don't call these methods directly, use the `Statusbar` namespace instead.
 */
namespace StatusbarOld
{
/**
 * \brief Initializes the legacy statusbar.
 */
void init();

/**
 * \brief Creates the statusbar window.
 * \return The statusbar window handle, or nullptr on failure.
 */
HWND create_window();

/**
 * \brief Destroys the statusbar window, if it exists.
 */
void destroy_window();

/**
 * \brief Gets the statusbar window handle, or nullptr if it doesn't exist.
 */
HWND hwnd();
} // namespace StatusbarOld
