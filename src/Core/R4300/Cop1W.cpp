/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include "R4300.hpp"
#include "Ops.hpp"
#include "Macros.hpp"

void CVT_S_W()
{
    if (check_cop1_unusable()) return;
    set_rounding();
    *reg_cop1_simple[core_cffd] = *((int32_t *)reg_cop1_simple[core_cffs]);
    PC++;
}

void CVT_D_W()
{
    if (check_cop1_unusable()) return;
    set_rounding();
    *reg_cop1_double[core_cffd] = *((int32_t *)reg_cop1_simple[core_cffs]);
    PC++;
}
