/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#ifdef _M_X64
#define VERSION_NAME_HELPER_ARCH L" x64"
#else
#define VERSION_NAME_HELPER_ARCH L" "
#endif

#ifdef _DEBUG
#define VERSION_NAME_HELPER_TARGET L" Debug"
#else
#define VERSION_NAME_HELPER_TARGET L" "
#endif

#define VERSION_NAME_HELPER_GEN_NAME(base_name, version)                                                               \
    base_name L" " version VERSION_NAME_HELPER_ARCH VERSION_NAME_HELPER_TARGET
