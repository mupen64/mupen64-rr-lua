/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <CommonPCH.h>
#include <core_api.h>
#include <Views.Win32/ViewPlugin.h>
// #include <PlatformService.h>

#define PLUGIN_VERSION L"1.0.0"

#ifdef _M_X64
#define PLUGIN_ARCH L" x64"
#else
#define PLUGIN_ARCH L" "
#endif

#ifdef _DEBUG
#define PLUGIN_TARGET L" Debug"
#else
#define PLUGIN_TARGET L" "
#endif

#define PLUGIN_NAME L"No RSP " PLUGIN_VERSION PLUGIN_ARCH PLUGIN_TARGET