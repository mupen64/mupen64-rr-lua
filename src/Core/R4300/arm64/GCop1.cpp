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

static void ctc1_apply_rounding()
{
    switch (FCR31 & 3)
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
}

void genmfc1()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.r.nrd * sizeof(reg_cop1_simple[0])));
    emit_ldrsw_off(A64_T2, A64_T1, 0);
    emit_reg_base();
    emit_str_x_off(A64_T2, A64_RB, reg_offset(dst->f.r.rt));
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)MFC1, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gendmfc1()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.r.nrd * sizeof(reg_cop1_double[0])));
    emit_ldr_x_off(A64_T2, A64_T1, 0);
    emit_reg_base();
    emit_str_x_off(A64_T2, A64_RB, reg_offset(dst->f.r.rt));
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)DMFC1, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gencfc1()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    const uint32_t fs = dst->f.r.nrd;
    if (fs == 31 || fs == 0)
    {
        emit_mov_reg_imm64(A64_T0, fs == 31 ? (uintptr_t)&FCR31 : (uintptr_t)&FCR0);
        emit_ldrsw_off(A64_T1, A64_T0, 0);
        emit_reg_base();
        emit_str_x_off(A64_T1, A64_RB, reg_offset(dst->f.r.rt));
    }

    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CFC1, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genmtc1()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_reg_base();
    emit_ldr_x_off(A64_T2, A64_RB, reg_offset(dst->f.r.rt));
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.r.nrd * sizeof(reg_cop1_simple[0])));
    emit_str_w_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)MTC1, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gendmtc1()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    emit_reg_base();
    emit_ldr_x_off(A64_T2, A64_RB, reg_offset(dst->f.r.rt));
    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.r.nrd * sizeof(reg_cop1_double[0])));
    emit_str_x_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)DMTC1, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genctc1()
{
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t to_slow = emit_tbz_w(A64_T1, 29, 0);

    if (dst->f.r.nrd == 31)
    {
        emit_reg_base();
        emit_ldr_w_off(A64_T1, A64_RB, reg_offset(dst->f.r.rt));
        emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
        emit_str_w_off(A64_T1, A64_T0, 0);
    }

    emit_mov_reg_imm64(A64_IP0, (uintptr_t)ctc1_apply_rounding);
    emit_blr(A64_IP0);

    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)CTC1, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}
