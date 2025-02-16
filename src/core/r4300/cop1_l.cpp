/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "r4300.h"
#include "fpu.h"
#include "macros.h"


void CVT_S_L(void)
{
    if (check_cop1_unusable()) return;
    cvt_s_l((long long*)(reg_cop1_double[core_cffs]), reg_cop1_simple[core_cffd]);
    PC++;
}

void CVT_D_L(void)
{
    if (check_cop1_unusable()) return;
    cvt_d_l((long long*)(reg_cop1_double[core_cffs]), reg_cop1_double[core_cffd]);
    PC++;
}
