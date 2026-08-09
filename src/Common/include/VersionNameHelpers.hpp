/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#define CURRENT_VERSION "1.5.0"

#ifdef _M_X64
#define VERSION_NAME_HELPER_ARCH " "
#else
#define VERSION_NAME_HELPER_ARCH " x86"
#endif

#ifdef _DEBUG
#define VERSION_NAME_HELPER_TARGET " Debug"
#else
#define VERSION_NAME_HELPER_TARGET ""
#endif

#define VERSION_NAME_HELPER_GEN_NAME(base_name)                                                                        \
    base_name " " CURRENT_VERSION VERSION_SUFFIX VERSION_NAME_HELPER_ARCH VERSION_NAME_HELPER_TARGET
