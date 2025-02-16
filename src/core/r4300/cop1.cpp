/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "ops.h"
#include "r4300.h"
#include "macros.h"

void MFC1(void)
{
    if (check_cop1_unusable())
        return;
    rrt32 = *((int*)reg_cop1_simple[core_rfs]);
    sign_extended(core_rrt);
    PC++;
}

void DMFC1(void)
{
    if (check_cop1_unusable())
        return;
    core_rrt = *((long long*)reg_cop1_double[core_rfs]);
    PC++;
}

void CFC1(void)
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

void MTC1(void)
{
    if (check_cop1_unusable())
        return;
    *((int*)reg_cop1_simple[core_rfs]) = rrt32;
    PC++;
}

void DMTC1(void)
{
    if (check_cop1_unusable())
        return;
    *((long long*)reg_cop1_double[core_rfs]) = core_rrt;
    PC++;
}

void CTC1(void)
{
    if (check_cop1_unusable())
        return;
    if (core_rfs == 31)
        FCR31 = rrt32;
    switch ((FCR31 & 3))
    {
    case 0:
        rounding_mode = 0x33F; // Round to nearest, or to even if equidistant
        break;
    case 1:
        rounding_mode = 0xF3F; // Truncate (toward 0)
        break;
    case 2:
        rounding_mode = 0xB3F; // Round up (toward +infinity)
        break;
    case 3:
        rounding_mode = 0x73F; // Round down (toward -infinity)
        break;
    }
    PC++;
}
