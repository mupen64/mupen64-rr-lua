/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <string>

/**
 * \brief A module responsible for implementing a hotkey detection and invocation mechanism.
 */
namespace HotkeyTracker
{
/**
 * \brief Attaches a hotkey tracker to a top-level window.
 * \param hwnd Handle to a top-level window.
 * \param window_context The concrete context identifying this window.
 * \return Whether the operation succeeded.
 */
bool attach(HWND hwnd, const std::string &window_context = "window.builtin.main");

/**
 * \brief Handles keyboard hotkeys before dialog navigation, including messages targeting child controls.
 * \return Whether a hotkey consumed the message.
 */
bool process_message(const MSG &msg);
} // namespace HotkeyTracker
