/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "r4300.h"
#include "ops.h"
#include "macros.h"

void CVT_S_W()
{
    if (check_cop1_unusable())
        return;
    set_rounding();
    *reg_cop1_simple[core_cffd] = *((int32_t*)reg_cop1_simple[core_cffs]);
    PC++;
}

void CVT_D_W()
{
    if (check_cop1_unusable())
        return;
    set_rounding();
    *reg_cop1_double[core_cffd] = *((int32_t*)reg_cop1_simple[core_cffs]);
    PC++;
}
