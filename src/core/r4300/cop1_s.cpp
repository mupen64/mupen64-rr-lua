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
#include <r4300/fpu.h>

void ADD_S(void)
{
   if (check_cop1_unusable()) return;
   add_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft], reg_cop1_simple[core_cffd]);
   PC++;
}

void SUB_S(void)
{
   if (check_cop1_unusable()) return;
   sub_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft], reg_cop1_simple[core_cffd]);
   PC++;
}

void MUL_S(void)
{
   if (check_cop1_unusable()) return;
   mul_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft], reg_cop1_simple[core_cffd]);
   PC++;
}

void DIV_S(void)
{  
   if (check_cop1_unusable()) return;
   div_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft], reg_cop1_simple[core_cffd]);
   PC++;
}

void SQRT_S(void)
{
   if (check_cop1_unusable()) return;
   sqrt_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
   PC++;
}

void ABS_S(void)
{
   if (check_cop1_unusable()) return;
   abs_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
   PC++;
}

void MOV_S(void)
{
   if (check_cop1_unusable()) return;
   mov_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
   PC++;
}

void NEG_S(void)
{
   if (check_cop1_unusable()) return;
   neg_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
   PC++;
}

void ROUND_L_S(void)
{
   if (check_cop1_unusable()) return;
   round_l_s(reg_cop1_simple[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   PC++;
}

void TRUNC_L_S(void)
{
   if (check_cop1_unusable()) return;
   trunc_l_s(reg_cop1_simple[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   PC++;
}

void CEIL_L_S(void)
{
   if (check_cop1_unusable()) return;
   ceil_l_s(reg_cop1_simple[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   PC++;
}

void FLOOR_L_S(void)
{
   if (check_cop1_unusable()) return;
   floor_l_s(reg_cop1_simple[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   PC++;
}

void ROUND_W_S(void)
{
   if (check_cop1_unusable()) return;
   round_w_s(reg_cop1_simple[core_cffs], (int*)reg_cop1_simple[core_cffd]);
   PC++;
}

void TRUNC_W_S(void)
{
   if (check_cop1_unusable()) return;
   trunc_w_s(reg_cop1_simple[core_cffs], (int*)reg_cop1_simple[core_cffd]);
   PC++;
}

void CEIL_W_S(void)
{
   if (check_cop1_unusable()) return;
   ceil_w_s(reg_cop1_simple[core_cffs], (int*)reg_cop1_simple[core_cffd]);
   PC++;
}

void FLOOR_W_S(void)
{
   if (check_cop1_unusable()) return;
   floor_w_s(reg_cop1_simple[core_cffs], (int*)reg_cop1_simple[core_cffd]);
   PC++;
}

void CVT_D_S(void)
{
   if (check_cop1_unusable()) return;
   cvt_d_s(reg_cop1_simple[core_cffs], reg_cop1_double[core_cffd]);
   PC++;
}

void CVT_W_S(void)
{
   if (check_cop1_unusable()) return;
   cvt_w_s(reg_cop1_simple[core_cffs], (int*)reg_cop1_simple[core_cffd]);
   PC++;
}

void CVT_L_S(void)
{
   if (check_cop1_unusable()) return;
   cvt_l_s(reg_cop1_simple[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   PC++;
}

void C_F_S(void)
{
   if (check_cop1_unusable()) return;
   c_f_s();
   PC++;
}

void C_UN_S(void)
{
   if (check_cop1_unusable()) return;
   c_un_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_EQ_S(void)
{
   if (check_cop1_unusable()) return;
   c_eq_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_UEQ_S(void)
{
   if (check_cop1_unusable()) return;
   c_ueq_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_OLT_S(void)
{
   if (check_cop1_unusable()) return;
   c_olt_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_ULT_S(void)
{
   if (check_cop1_unusable()) return;
   c_ult_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_OLE_S(void)
{
   if (check_cop1_unusable()) return;
   c_ole_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_ULE_S(void)
{
   if (check_cop1_unusable()) return;
   c_ule_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_SF_S(void)
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_sf_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_NGLE_S(void)
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_ngle_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_SEQ_S(void)
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_seq_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_NGL_S(void)
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_ngl_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_LT_S(void)
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_lt_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_NGE_S(void)
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_nge_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_LE_S(void)
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_le_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

void C_NGT_S(void)
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_ngt_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   PC++;
}

