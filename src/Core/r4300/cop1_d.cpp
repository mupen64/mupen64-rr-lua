/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <r4300/cop1_helpers.h>
#include <r4300/macros.h>
#include <r4300/ops.h>
#include <r4300/r4300.h>

void ADD_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    CHECK_INPUT(*reg_cop1_double[core_cfft]);
    *reg_cop1_double[core_cffd] = *reg_cop1_double[core_cffs] +
    *reg_cop1_double[core_cfft];
    CHECK_OUTPUT(*reg_cop1_double[core_cffd]);
    PC++;
}

void SUB_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    CHECK_INPUT(*reg_cop1_double[core_cfft]);
    *reg_cop1_double[core_cffd] = *reg_cop1_double[core_cffs] -
    *reg_cop1_double[core_cfft];
    CHECK_OUTPUT(*reg_cop1_double[core_cffd]);
    PC++;
}

void MUL_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    CHECK_INPUT(*reg_cop1_double[core_cfft]);
    *reg_cop1_double[core_cffd] = *reg_cop1_double[core_cffs] *
    *reg_cop1_double[core_cfft];
    CHECK_OUTPUT(*reg_cop1_double[core_cffd]);
    PC++;
}

void DIV_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    CHECK_INPUT(*reg_cop1_double[core_cfft]);
    *reg_cop1_double[core_cffd] = *reg_cop1_double[core_cffs] /
    *reg_cop1_double[core_cfft];
    CHECK_OUTPUT(*reg_cop1_double[core_cffd]);
    PC++;
}

void SQRT_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    *reg_cop1_double[core_cffd] = sqrt(*reg_cop1_double[core_cffs]);
    CHECK_OUTPUT(*reg_cop1_double[core_cffd]);
    PC++;
}

void ABS_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    *reg_cop1_double[core_cffd] = fabs(*reg_cop1_double[core_cffs]);
    // ABS cannot fail
    PC++;
}

void MOV_D()
{
    if (check_cop1_unusable())
        return;
    // MOV is not an arithmetic instruction, no check needed
    *reg_cop1_double[core_cffd] = *reg_cop1_double[core_cffs];
    PC++;
}

void NEG_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    *reg_cop1_double[core_cffd] = -(*reg_cop1_double[core_cffs]);
    // NEG cannot fail
    PC++;
}

void ROUND_L_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    set_round_to_nearest();
    clear_x87_exceptions();
    FLOAT_CONVERT_L_D(reg_cop1_double[core_cffs], reg_cop1_double[core_cffd]);
    set_rounding();
    PC++;
}

void TRUNC_L_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    set_trunc();
    clear_x87_exceptions();
    FLOAT_CONVERT_L_D(reg_cop1_double[core_cffs], reg_cop1_double[core_cffd]);
    set_rounding();
    PC++;
}

void CEIL_L_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    set_ceil();
    clear_x87_exceptions();
    FLOAT_CONVERT_L_D(reg_cop1_double[core_cffs], reg_cop1_double[core_cffd]);
    set_rounding();
    PC++;
}

void FLOOR_L_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    set_floor();
    clear_x87_exceptions();
    FLOAT_CONVERT_L_D(reg_cop1_double[core_cffs], reg_cop1_double[core_cffd]);
    set_rounding();
    PC++;
}

void ROUND_W_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    set_round_to_nearest();
    clear_x87_exceptions();
    FLOAT_CONVERT_W_D(reg_cop1_double[core_cffs], reg_cop1_simple[core_cffd]);
    set_rounding();
    PC++;
}

void TRUNC_W_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    set_trunc();
    clear_x87_exceptions();
    FLOAT_CONVERT_W_D(reg_cop1_double[core_cffs], reg_cop1_simple[core_cffd]);
    set_rounding();
    PC++;
}

void CEIL_W_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    set_ceil();
    clear_x87_exceptions();
    FLOAT_CONVERT_W_D(reg_cop1_double[core_cffs], reg_cop1_simple[core_cffd]);
    set_rounding();
    PC++;
}

void FLOOR_W_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    set_floor();
    clear_x87_exceptions();
    FLOAT_CONVERT_W_D(reg_cop1_double[core_cffs], reg_cop1_simple[core_cffd]);
    set_rounding();
    PC++;
}

void CVT_S_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    if (g_core->cfg->wii_vc_emulation)
    {
        set_trunc();
    }
    *reg_cop1_simple[core_cffd] = *reg_cop1_double[core_cffs];
    if (g_core->cfg->wii_vc_emulation)
    {
        set_rounding();
    }
    CHECK_OUTPUT(*reg_cop1_simple[core_cffd]);
    PC++;
}

void CVT_W_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    clear_x87_exceptions();
    FLOAT_CONVERT_W_D(reg_cop1_double[core_cffs], reg_cop1_simple[core_cffd]);
    PC++;
}

void CVT_L_D()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_double[core_cffs]);
    clear_x87_exceptions();
    FLOAT_CONVERT_L_D(reg_cop1_double[core_cffs], reg_cop1_double[core_cffd]);
    PC++;
}

void C_F_D()
{
    if (check_cop1_unusable())
        return;
    FCR31 &= ~0x800000;
    PC++;
}

void C_UN_D()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_EQ_D()
{
    if (check_cop1_unusable())
        return;
    if (!isnan(*reg_cop1_double[core_cffs]) && !isnan(*reg_cop1_double[core_cfft]) &&
        *reg_cop1_double[core_cffs] == *reg_cop1_double[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_UEQ_D()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]) ||
        *reg_cop1_double[core_cffs] == *reg_cop1_double[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_OLT_D()
{
    if (check_cop1_unusable())
        return;
    if (!isnan(*reg_cop1_double[core_cffs]) && !isnan(*reg_cop1_double[core_cfft]) &&
        *reg_cop1_double[core_cffs] < *reg_cop1_double[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_ULT_D()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]) ||
        *reg_cop1_double[core_cffs] < *reg_cop1_double[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_OLE_D()
{
    if (check_cop1_unusable())
        return;
    if (!isnan(*reg_cop1_double[core_cffs]) && !isnan(*reg_cop1_double[core_cfft]) &&
        *reg_cop1_double[core_cffs] <= *reg_cop1_double[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_ULE_D()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]) ||
        *reg_cop1_double[core_cffs] <= *reg_cop1_double[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_SF_D()
{
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->log_error(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    FCR31 &= ~0x800000;
    PC++;
}

void C_NGLE_D()
{
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->log_error(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    FCR31 &= ~0x800000;
    PC++;
}

void C_SEQ_D()
{
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->log_error(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    if (*reg_cop1_double[core_cffs] == *reg_cop1_double[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_NGL_D()
{
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->log_error(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    if (*reg_cop1_double[core_cffs] == *reg_cop1_double[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_LT_D()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->log_error(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    if (*reg_cop1_double[core_cffs] < *reg_cop1_double[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_NGE_D()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->log_info(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    if (*reg_cop1_double[core_cffs] < *reg_cop1_double[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_LE_D()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->log_info(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    if (*reg_cop1_double[core_cffs] <= *reg_cop1_double[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_NGT_D()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->log_info(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    if (*reg_cop1_double[core_cffs] <= *reg_cop1_double[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}
