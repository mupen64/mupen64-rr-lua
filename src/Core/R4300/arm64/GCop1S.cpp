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

void genadd_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)ADD_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);

    emit_fadd(0, 0, 1, false);

    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_fstr(0, A64_T1, false);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)ADD_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gensub_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)SUB_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);

    emit_fsub(0, 0, 1, false);

    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_fstr(0, A64_T1, false);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)SUB_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genmul_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)MUL_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);

    emit_fmul(0, 0, 1, false);

    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_fstr(0, A64_T1, false);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)MUL_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gendiv_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)DIV_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);

    emit_fdiv(0, 0, 1, false);

    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_fstr(0, A64_T1, false);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)DIV_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gensqrt_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)SQRT_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_fsqrt(0, 0, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_fstr(0, A64_T1, false);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)SQRT_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genabs_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)ABS_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_fabs(0, 0, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_fstr(0, A64_T1, false);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)ABS_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genmov_s()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_fmov(0, 0, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_fstr(0, A64_T1, false);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)MOV_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genneg_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)NEG_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_fneg(0, 0, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_fstr(0, A64_T1, false);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)NEG_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genround_l_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)ROUND_L_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_NEAREST, false, true);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_str_x_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)ROUND_L_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gentrunc_l_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)TRUNC_L_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_ZERO, false, true);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_str_x_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)TRUNC_L_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genceil_l_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)CEIL_L_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_PLUS_INF, false, true);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_str_x_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CEIL_L_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genfloor_l_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)FLOOR_L_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_MINUS_INF, false, true);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_str_x_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)FLOOR_L_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genround_w_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)ROUND_W_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_NEAREST, false, false);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_str_w_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)ROUND_W_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gentrunc_w_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)TRUNC_W_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_ZERO, false, false);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_str_w_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)TRUNC_W_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genceil_w_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)CEIL_W_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_PLUS_INF, false, false);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_str_w_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CEIL_W_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genfloor_w_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)FLOOR_W_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_MINUS_INF, false, false);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_str_w_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)FLOOR_W_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gencvt_d_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)CVT_D_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_fcvt_s_to_d(0, 0);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_fstr(0, A64_T1, true);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CVT_D_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gencvt_w_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)CVT_W_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_frintx(0, 0, false);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_ZERO, false, false);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_str_w_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CVT_W_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gencvt_l_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)CVT_L_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_frintx(0, 0, false);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_ZERO, false, true);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_str_x_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CVT_L_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_f_s()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T2, 0);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_F_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_un_s()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);
    emit_fcmp(0, 1, false);
    emit_cset(A64_T2, A64_COND_VS);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_UN_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_eq_s()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);
    emit_fcmp(0, 1, false);
    emit_cset(A64_T2, A64_COND_EQ);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_EQ_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ueq_s()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);
    emit_fcmp(0, 1, false);
    emit_cset(A64_T2, A64_COND_EQ);
    emit_cset(A64_T3, A64_COND_VS);
    emit_orr(A64_T2, A64_T2, A64_T3, false);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_UEQ_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_olt_s()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);
    emit_fcmp(0, 1, false);
    emit_cset(A64_T2, A64_COND_MI);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_OLT_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ult_s()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);
    emit_fcmp(0, 1, false);
    emit_cset(A64_T2, A64_COND_LT);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_ULT_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ole_s()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);
    emit_fcmp(0, 1, false);
    emit_cset(A64_T2, A64_COND_LS);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_OLE_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ule_s()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);
    emit_fcmp(0, 1, false);
    emit_cset(A64_T2, A64_COND_LE);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_ULE_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_sf_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_SF_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T2, 0);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_SF_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ngle_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_NGLE_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T2, 0);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_NGLE_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_seq_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_SEQ_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);
    emit_fcmp(0, 1, false);
    emit_cset(A64_T2, A64_COND_EQ);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_SEQ_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ngl_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_NGL_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);
    emit_fcmp(0, 1, false);
    emit_cset(A64_T2, A64_COND_EQ);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_NGL_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_lt_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_LT_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);
    emit_fcmp(0, 1, false);
    emit_cset(A64_T2, A64_COND_MI);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_LT_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_nge_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_NGE_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);
    emit_fcmp(0, 1, false);
    emit_cset(A64_T2, A64_COND_MI);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_NGE_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_le_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_LE_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);
    emit_fcmp(0, 1, false);
    emit_cset(A64_T2, A64_COND_LS);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_LE_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ngt_s()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_NGT_S, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_simple[0])));
    emit_fldr(0, A64_T1, false);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_simple[0])));
    emit_fldr(1, A64_T1, false);
    emit_fcmp(0, 1, false);
    emit_cset(A64_T2, A64_COND_LS);
    emit_lsl_imm_w(A64_T2, A64_T2, 23);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T3, 0xFF7FFFFFu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_orr(A64_T1, A64_T1, A64_T2, false);
    emit_str_w_off(A64_T1, A64_T0, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)C_NGT_S, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}
