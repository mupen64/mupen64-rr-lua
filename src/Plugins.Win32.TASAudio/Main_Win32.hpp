/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <CommonPCH.h>
#include "Config.hpp"

extern HINSTANCE g_dll_handle;

// Reads config info from the Win32 registry.
SDLAudio::Config win32_read_config();

// Writes config info to the Win32 registry.
void win32_write_config(const SDLAudio::Config &cfg);
