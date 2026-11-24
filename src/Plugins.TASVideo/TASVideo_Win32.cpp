/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ConfigDialog_Win32.hpp"
#include "Core/Plugin.hpp"

namespace BuiltinTAS::TASVideo
{
EXPORT void CALL M64RRShowConfig(M64RRSpec::WindowHandle parent_window)
{
    ::TASVideo::ConfigDialog::show(parent_window.hwnd());
}
} // namespace BuiltinTAS::TASVideo
