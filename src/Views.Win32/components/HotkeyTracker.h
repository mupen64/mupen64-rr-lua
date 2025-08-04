/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing a hotkey detection and invocation mechanism.
 */
namespace HotkeyTracker
{
    /**
     * \brief Attaches a hotkey tracker to a top-level window.
     * \param hwnd Handle to a top-level window.
     * \return Whether the operation succeeded.
     */
    bool attach(HWND hwnd);
} // namespace HotkeyTracker
