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

void genadd_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)ADD_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);

    emit_fadd(0, 0, 1, true);

    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_fstr(0, A64_T1, true);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)ADD_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gensub_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)SUB_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);

    emit_fsub(0, 0, 1, true);

    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_fstr(0, A64_T1, true);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)SUB_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genmul_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)MUL_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);

    emit_fmul(0, 0, 1, true);

    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_fstr(0, A64_T1, true);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)MUL_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gendiv_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)DIV_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);

    emit_fdiv(0, 0, 1, true);

    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_fstr(0, A64_T1, true);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)DIV_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gensqrt_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)SQRT_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_fsqrt(0, 0, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_fstr(0, A64_T1, true);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)SQRT_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genabs_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)ABS_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_fabs(0, 0, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_fstr(0, A64_T1, true);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)ABS_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genmov_d()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_fmov(0, 0, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_fstr(0, A64_T1, true);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)MOV_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genneg_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)NEG_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_fneg(0, 0, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_fstr(0, A64_T1, true);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)NEG_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genround_l_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)ROUND_L_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_NEAREST, true, true);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_str_x_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)ROUND_L_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gentrunc_l_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)TRUNC_L_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_ZERO, true, true);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_str_x_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)TRUNC_L_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genceil_l_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)CEIL_L_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_PLUS_INF, true, true);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_str_x_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CEIL_L_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genfloor_l_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)FLOOR_L_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_MINUS_INF, true, true);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_str_x_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)FLOOR_L_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genround_w_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)ROUND_W_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_NEAREST, true, false);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_str_w_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)ROUND_W_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gentrunc_w_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)TRUNC_W_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_ZERO, true, false);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_str_w_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)TRUNC_W_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genceil_w_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)CEIL_W_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_PLUS_INF, true, false);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_str_w_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CEIL_W_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genfloor_w_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)FLOOR_W_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_MINUS_INF, true, false);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_str_w_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)FLOOR_W_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gencvt_s_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)CVT_S_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_fcvt_d_to_s(0, 0);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_fstr(0, A64_T1, false);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CVT_S_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gencvt_w_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)CVT_W_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_frintx(0, 0, true);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_ZERO, true, false);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_simple[0])));
    emit_str_w_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CVT_W_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gencvt_l_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)CVT_L_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_frintx(0, 0, true);
    emit_fcvt_to_int(A64_T2, 0, A64_RMODE_ZERO, true, true);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fd * sizeof(reg_cop1_double[0])));
    emit_str_x_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CVT_L_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_f_d()
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
    gencallinterp((uintptr_t)C_F_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_un_d()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);
    emit_fcmp(0, 1, true);
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
    gencallinterp((uintptr_t)C_UN_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_eq_d()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);
    emit_fcmp(0, 1, true);
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
    gencallinterp((uintptr_t)C_EQ_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ueq_d()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);
    emit_fcmp(0, 1, true);
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
    gencallinterp((uintptr_t)C_UEQ_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_olt_d()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);
    emit_fcmp(0, 1, true);
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
    gencallinterp((uintptr_t)C_OLT_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ult_d()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);
    emit_fcmp(0, 1, true);
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
    gencallinterp((uintptr_t)C_ULT_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ole_d()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);
    emit_fcmp(0, 1, true);
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
    gencallinterp((uintptr_t)C_OLE_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ule_d()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);
    emit_fcmp(0, 1, true);
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
    gencallinterp((uintptr_t)C_ULE_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_sf_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_SF_D, 0);
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
    gencallinterp((uintptr_t)C_SF_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ngle_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_NGLE_D, 0);
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
    gencallinterp((uintptr_t)C_NGLE_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_seq_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_SEQ_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);
    emit_fcmp(0, 1, true);
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
    gencallinterp((uintptr_t)C_SEQ_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ngl_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_NGL_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);
    emit_fcmp(0, 1, true);
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
    gencallinterp((uintptr_t)C_NGL_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_lt_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_LT_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);
    emit_fcmp(0, 1, true);
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
    gencallinterp((uintptr_t)C_LT_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_nge_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_NGE_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);
    emit_fcmp(0, 1, true);
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
    gencallinterp((uintptr_t)C_NGE_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_le_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_LE_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);
    emit_fcmp(0, 1, true);
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
    gencallinterp((uintptr_t)C_LE_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genc_ngt_d()
{
    free_all_registers();

    if (g_core->cfg->float_exception_emulation)
    {
        gencallinterp((uintptr_t)C_NGT_D, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.fs * sizeof(reg_cop1_double[0])));
    emit_fldr(0, A64_T1, true);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.cf.ft * sizeof(reg_cop1_double[0])));
    emit_fldr(1, A64_T1, true);
    emit_fcmp(0, 1, true);
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
    gencallinterp((uintptr_t)C_NGT_D, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}
