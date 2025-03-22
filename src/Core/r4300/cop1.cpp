/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "ops.h"
#include "r4300.h"
#include "macros.h"

void MFC1()
{
    if (check_cop1_unusable())
        return;
    rrt32 = *((int32_t*)reg_cop1_simple[core_rfs]);
    sign_extended(core_rrt);
    PC++;
}

void DMFC1()
{
    if (check_cop1_unusable())
        return;
    core_rrt = *((int64_t*)reg_cop1_double[core_rfs]);
    PC++;
}

void CFC1()
{
    if (check_cop1_unusable())
        return;
    if (core_rfs == 31)
    {
        rrt32 = FCR31;
        sign_extended(core_rrt);
    }
    if (core_rfs == 0)
    {
        rrt32 = FCR0;
        sign_extended(core_rrt);
    }
    PC++;
}

void MTC1()
{
    if (check_cop1_unusable())
        return;
    *((int32_t*)reg_cop1_simple[core_rfs]) = rrt32;
    PC++;
}

void DMTC1()
{
    if (check_cop1_unusable())
        return;
    *((int64_t*)reg_cop1_double[core_rfs]) = core_rrt;
    PC++;
}

void CTC1()
{
    if (check_cop1_unusable())
        return;
    if (core_rfs == 31)
        FCR31 = rrt32;
    switch ((FCR31 & 3))
    {
    case 0:
        rounding_mode = MUP_ROUND_NEAREST;
        break;
    case 1:
        rounding_mode = MUP_ROUND_TRUNC;
        break;
    case 2:
        rounding_mode = MUP_ROUND_CEIL;
        break;
    case 3:
        rounding_mode = MUP_ROUND_FLOOR;
        break;
    }
    set_rounding();
    // if ((FCR31 >> 7) & 0x1F) g_core->log_info(L"FPU Exception enabled : {:#06x}",
    //				   (int32_t)((FCR31 >> 7) & 0x1F));
    PC++;
}
