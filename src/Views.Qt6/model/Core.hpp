/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MODEL_CORE_HPP_INCLUDED
#define MODEL_CORE_HPP_INCLUDED

#include <core_api.h>

namespace Mupen
{
    extern core_cfg g_core_cfg;
    extern core_params g_core_params;
    extern core_ctx* g_core_ctx;
    void init_core(core_cfg config);
} // namespace Mupen

#endif