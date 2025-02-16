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
#include <r4300/fpu.h>

void ADD_D(void)
{
    if (check_cop1_unusable())
        return;
    add_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft], reg_cop1_double[core_cffd]);
    PC++;
}

void SUB_D(void)
{
    if (check_cop1_unusable())
        return;
    sub_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft], reg_cop1_double[core_cffd]);
    PC++;
}

void MUL_D(void)
{
    if (check_cop1_unusable())
        return;
    mul_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft], reg_cop1_double[core_cffd]);
    PC++;
}

void DIV_D(void)
{
    if (check_cop1_unusable())
        return;
    div_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft], reg_cop1_double[core_cffd]);
    PC++;
}

void SQRT_D(void)
{
    if (check_cop1_unusable())
        return;
    sqrt_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cffd]);
    PC++;
}

void ABS_D(void)
{
    if (check_cop1_unusable())
        return;
    abs_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cffd]);
    PC++;
}

void MOV_D(void)
{
    if (check_cop1_unusable())
        return;
    mov_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cffd]);
    PC++;
}

void NEG_D(void)
{
    if (check_cop1_unusable())
        return;
    neg_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cffd]);
    PC++;
}

void ROUND_L_D(void)
{
    if (check_cop1_unusable())
        return;
    round_l_d(reg_cop1_double[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
    PC++;
}

void TRUNC_L_D(void)
{
    if (check_cop1_unusable())
        return;
    trunc_l_d(reg_cop1_double[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
    PC++;
}

void CEIL_L_D(void)
{
    if (check_cop1_unusable())
        return;
    ceil_l_d(reg_cop1_double[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
    PC++;
}

void FLOOR_L_D(void)
{
    if (check_cop1_unusable())
        return;
    floor_l_d(reg_cop1_double[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
    PC++;
}

void ROUND_W_D(void)
{
    if (check_cop1_unusable())
        return;
    round_w_d(reg_cop1_double[core_cffs], (int*)reg_cop1_simple[core_cffd]);
    PC++;
}

void TRUNC_W_D(void)
{
    if (check_cop1_unusable())
        return;
    trunc_w_d(reg_cop1_double[core_cffs], (int*)reg_cop1_simple[core_cffd]);
    PC++;
}

void CEIL_W_D(void)
{
    if (check_cop1_unusable())
        return;
    ceil_w_d(reg_cop1_double[core_cffs], (int*)reg_cop1_simple[core_cffd]);
    PC++;
}

void FLOOR_W_D(void)
{
    if (check_cop1_unusable())
        return;
    floor_w_d(reg_cop1_double[core_cffs], (int*)reg_cop1_simple[core_cffd]);
    PC++;
}

void CVT_S_D(void)
{
    if (check_cop1_unusable())
        return;
    cvt_s_d(reg_cop1_double[core_cffs], reg_cop1_simple[core_cffd]);
    PC++;
}

void CVT_W_D(void)
{
    if (check_cop1_unusable())
        return;
    cvt_w_d(reg_cop1_double[core_cffs], (int*)reg_cop1_simple[core_cffd]);
    PC++;
}

void CVT_L_D(void)
{
    if (check_cop1_unusable())
        return;
    cvt_l_d(reg_cop1_double[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
    PC++;
}

void C_F_D(void)
{
    if (check_cop1_unusable())
        return;
    c_f_d();
    PC++;
}

void C_UN_D(void)
{
    if (check_cop1_unusable())
        return;
    c_un_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_EQ_D(void)
{
    if (check_cop1_unusable())
        return;
    c_eq_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_UEQ_D(void)
{
    if (check_cop1_unusable())
        return;
    c_ueq_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_OLT_D(void)
{
    if (check_cop1_unusable())
        return;
    c_olt_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_ULT_D(void)
{
    if (check_cop1_unusable())
        return;
    c_ult_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_OLE_D(void)
{
    if (check_cop1_unusable())
        return;
    c_ole_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_ULE_D(void)
{
    if (check_cop1_unusable())
        return;
    c_ule_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_SF_D(void)
{
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->logger->error("Invalid operation exception in C opcode");
        stop = 1;
    }
    c_sf_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_NGLE_D(void)
{
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->logger->error("Invalid operation exception in C opcode");
        stop = 1;
    }
    c_ngle_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_SEQ_D(void)
{
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->logger->error("Invalid operation exception in C opcode");
        stop = 1;
    }
    c_seq_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_NGL_D(void)
{
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->logger->error("Invalid operation exception in C opcode");
        stop = 1;
    }
    c_ngl_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_LT_D(void)
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->logger->error("Invalid operation exception in C opcode");
        stop = 1;
    }
    c_lt_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_NGE_D(void)
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->logger->error("Invalid operation exception in C opcode");
        stop = 1;
    }
    c_nge_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_LE_D(void)
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->logger->error("Invalid operation exception in C opcode");
        stop = 1;
    }
    c_le_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}

void C_NGT_D(void)
{
    if (check_cop1_unusable())
        return;
    if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
    {
        g_core->logger->error("Invalid operation exception in C opcode");
        stop = 1;
    }
    c_ngt_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
    PC++;
}
