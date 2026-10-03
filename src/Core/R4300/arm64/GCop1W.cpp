/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Core.hpp>
#include <R4300/Exception.hpp>
#include <R4300/Ops.hpp>
#include <R4300/Macros.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomp.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/arm64/Assemble.hpp>
#include <R4300/arm64/RegCache.hpp>

void gencvt_s_w()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)CVT_S_W, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_ldrsw_off(A64_T2, A64_T1, 0);
    emit_scvtf(0, A64_T2, false, false);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_fstr(0, A64_T1, false);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CVT_S_W, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gencvt_d_w()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)CVT_D_W, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_ldrsw_off(A64_T2, A64_T1, 0);
    emit_scvtf(0, A64_T2, true, false);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_fstr(0, A64_T1, true);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CVT_D_W, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}
