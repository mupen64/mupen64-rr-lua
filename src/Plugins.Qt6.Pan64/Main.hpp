/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <CommonPCH.h>
#include <core_api.h>
#include <mupapi.h>

#define PLUGIN_VERSION "1.0.0"

#if defined(_M_X64) || defined(__x86_64__)
#define PLUGIN_ARCH " x64"
#else
#define PLUGIN_ARCH " "
#endif

#ifdef _DEBUG
#define PLUGIN_TARGET " Debug"
#else
#define PLUGIN_TARGET " "
#endif

#define PLUGIN_NAME "PAN64 " PLUGIN_VERSION PLUGIN_ARCH PLUGIN_TARGET

// constexpr uint32_t 

extern core_audio_info g_core_info;