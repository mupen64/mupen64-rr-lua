/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Config.hpp"

namespace SDLAudio
{
// Displays a Win32 dialog box for the provided config object.
bool win32_show_config(HWND parent, Config &config);
} // namespace SDLAudio