/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include "ConfigDialog_Win32.hpp"

EXPORT void CALL M64RRShowConfig(WindowHandle parent_window)
{
    TASVideo::ConfigDialog::show(parent_window.hwnd());
}
