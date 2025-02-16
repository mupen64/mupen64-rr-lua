/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "r4300.h"
#include "ops.h"
#include "macros.h"
#include "fpu.h"

void CVT_S_W(void)
{  
    if (check_cop1_unusable()) return;
    cvt_s_w((int*)reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
    PC++;
}

void CVT_D_W(void)
{
    if (check_cop1_unusable()) return;
    cvt_d_w((int*)reg_cop1_simple[core_cffs], reg_cop1_double[core_cffd]);
    PC++;
}

