/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <r4300/r4300.h>
#include <r4300/ops.h>
#include <r4300/macros.h>
#include <r4300/cop1_helpers.h>

void ADD_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    CHECK_INPUT(*reg_cop1_simple[core_cfft]);
    *reg_cop1_simple[core_cffd] = *reg_cop1_simple[core_cffs] +
    *reg_cop1_simple[core_cfft];
    CHECK_OUTPUT(*reg_cop1_simple[core_cffd]);
    PC++;
}

void SUB_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    CHECK_INPUT(*reg_cop1_simple[core_cfft]);
    *reg_cop1_simple[core_cffd] = *reg_cop1_simple[core_cffs] -
    *reg_cop1_simple[core_cfft];
    CHECK_OUTPUT(*reg_cop1_simple[core_cffd]);
    PC++;
}

void MUL_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    CHECK_INPUT(*reg_cop1_simple[core_cfft]);
    *reg_cop1_simple[core_cffd] = *reg_cop1_simple[core_cffs] *
    *reg_cop1_simple[core_cfft];
    CHECK_OUTPUT(*reg_cop1_simple[core_cffd]);
    PC++;
}

void DIV_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    CHECK_INPUT(*reg_cop1_simple[core_cfft]);
    *reg_cop1_simple[core_cffd] = *reg_cop1_simple[core_cffs] /
    *reg_cop1_simple[core_cfft];
    CHECK_OUTPUT(*reg_cop1_simple[core_cffd]);
    PC++;
}

void SQRT_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    *reg_cop1_simple[core_cffd] = sqrt(*reg_cop1_simple[core_cffs]);
    CHECK_OUTPUT(*reg_cop1_simple[core_cffd]);
    PC++;
}

void ABS_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    *reg_cop1_simple[core_cffd] = fabs(*reg_cop1_simple[core_cffs]);
    // ABS cannot fail
    PC++;
}

void MOV_S()
{
    if (check_cop1_unusable())
        return;
    // MOV is not an arithmetic instruction, no check needed
    *reg_cop1_simple[core_cffd] = *reg_cop1_simple[core_cffs];
    PC++;
}

void NEG_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    *reg_cop1_simple[core_cffd] = -(*reg_cop1_simple[core_cffs]);
    // NEG cannot fail
    PC++;
}

void ROUND_L_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    set_round_to_nearest();
    clear_x87_exceptions();
    FLOAT_CONVERT_L_S(reg_cop1_simple[core_cffs], reg_cop1_double[core_cffd]);
    set_rounding();
    CHECK_CONVERT_EXCEPTIONS();
    PC++;
}

void TRUNC_L_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    set_trunc();
    clear_x87_exceptions();
    FLOAT_CONVERT_L_S(reg_cop1_simple[core_cffs], reg_cop1_double[core_cffd]);
    set_rounding();
    CHECK_CONVERT_EXCEPTIONS();
    PC++;
}

void CEIL_L_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    set_ceil();
    clear_x87_exceptions();
    FLOAT_CONVERT_L_S(reg_cop1_simple[core_cffs], reg_cop1_double[core_cffd]);
    set_rounding();
    CHECK_CONVERT_EXCEPTIONS();
    PC++;
}

void FLOOR_L_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    set_floor();
    clear_x87_exceptions();
    FLOAT_CONVERT_L_S(reg_cop1_simple[core_cffs], reg_cop1_double[core_cffd]);
    set_rounding();
    CHECK_CONVERT_EXCEPTIONS();
    PC++;
}

void ROUND_W_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    set_round_to_nearest();
    clear_x87_exceptions();
    FLOAT_CONVERT_W_S(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
    set_rounding();
    CHECK_CONVERT_EXCEPTIONS();
    PC++;
}

void TRUNC_W_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    set_trunc();
    clear_x87_exceptions();
    FLOAT_CONVERT_W_S(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
    set_rounding();
    CHECK_CONVERT_EXCEPTIONS();
    PC++;
}

void CEIL_W_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    set_ceil();
    clear_x87_exceptions();
    FLOAT_CONVERT_W_S(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
    set_rounding();
    CHECK_CONVERT_EXCEPTIONS();
    PC++;
}

void FLOOR_W_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    set_floor();
    clear_x87_exceptions();
    FLOAT_CONVERT_W_S(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
    set_rounding();
    CHECK_CONVERT_EXCEPTIONS();
    PC++;
}

void CVT_D_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    *reg_cop1_double[core_cffd] = *reg_cop1_simple[core_cffs];
    PC++;
}

void CVT_W_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    clear_x87_exceptions();
    FLOAT_CONVERT_W_S(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
    CHECK_CONVERT_EXCEPTIONS();
    PC++;
}

void CVT_L_S()
{
    if (check_cop1_unusable())
        return;
    CHECK_INPUT(*reg_cop1_simple[core_cffs]);
    clear_x87_exceptions();
    FLOAT_CONVERT_L_S(reg_cop1_simple[core_cffs], reg_cop1_double[core_cffd]);
    CHECK_CONVERT_EXCEPTIONS();
    PC++;
}

void C_F_S()
{
    if (check_cop1_unusable())
        return;
    FCR31 &= ~0x800000;
    PC++;
}

void C_UN_S()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_EQ_S()
{
    if (check_cop1_unusable())
        return;
    if (!isnan(*reg_cop1_simple[core_cffs]) && !isnan(*reg_cop1_simple[core_cfft]) &&
        *reg_cop1_simple[core_cffs] == *reg_cop1_simple[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_UEQ_S()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]) ||
        *reg_cop1_simple[core_cffs] == *reg_cop1_simple[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_OLT_S()
{
    if (check_cop1_unusable())
        return;
    if (!isnan(*reg_cop1_simple[core_cffs]) && !isnan(*reg_cop1_simple[core_cfft]) &&
        *reg_cop1_simple[core_cffs] < *reg_cop1_simple[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_ULT_S()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]) ||
        *reg_cop1_simple[core_cffs] < *reg_cop1_simple[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_OLE_S()
{
    if (check_cop1_unusable())
        return;
    if (!isnan(*reg_cop1_simple[core_cffs]) && !isnan(*reg_cop1_simple[core_cfft]) &&
        *reg_cop1_simple[core_cffs] <= *reg_cop1_simple[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_ULE_S()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]) ||
        *reg_cop1_simple[core_cffs] <= *reg_cop1_simple[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_SF_S()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
    {
        g_core->log_error(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    FCR31 &= ~0x800000;
    PC++;
}

void C_NGLE_S()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
    {
        g_core->log_error(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    FCR31 &= ~0x800000;
    PC++;
}

void C_SEQ_S()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
    {
        g_core->log_error(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    if (*reg_cop1_simple[core_cffs] == *reg_cop1_simple[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_NGL_S()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
    {
        g_core->log_error(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    if (*reg_cop1_simple[core_cffs] == *reg_cop1_simple[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_LT_S()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
    {
        g_core->log_error(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    if (*reg_cop1_simple[core_cffs] < *reg_cop1_simple[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_NGE_S()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
    {
        g_core->log_error(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    if (*reg_cop1_simple[core_cffs] < *reg_cop1_simple[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_LE_S()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
    {
        g_core->log_info(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    if (*reg_cop1_simple[core_cffs] <= *reg_cop1_simple[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}

void C_NGT_S()
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
    {
        g_core->log_info(L"Invalid operation exception in C opcode");
        stop = 1;
    }
    if (*reg_cop1_simple[core_cffs] <= *reg_cop1_simple[core_cfft])
        FCR31 |= 0x800000;
    else
        FCR31 &= ~0x800000;
    PC++;
}
