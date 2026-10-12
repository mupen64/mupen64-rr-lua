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

void genbc1f()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1F, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)BC1F, 1);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T2, 0x800000u);
    emit_and(A64_T1, A64_T1, A64_T2, false);
    emit_cmp_w(A64_T1, A64_ZR);
    emit_cset(A64_T2, A64_COND_EQ);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&branch_taken);
    emit_str_w_off(A64_T2, A64_T1, 0);

    gendelayslot();
    gentest();
}

void genbc1f_out()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1F_OUT, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)BC1F_OUT, 1);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T2, 0x800000u);
    emit_and(A64_T1, A64_T1, A64_T2, false);
    emit_cmp_w(A64_T1, A64_ZR);
    emit_cset(A64_T2, A64_COND_EQ);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&branch_taken);
    emit_str_w_off(A64_T2, A64_T1, 0);

    gendelayslot();
    gentest_out();
}

void genbc1f_idle()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1F_IDLE, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)BC1F_IDLE, 1);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T2, 0x800000u);
    emit_and(A64_T1, A64_T1, A64_T2, false);
    emit_cmp_w(A64_T1, A64_ZR);
    emit_cset(A64_T2, A64_COND_EQ);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&branch_taken);
    emit_str_w_off(A64_T2, A64_T1, 0);

    gentest_idle();

    genbc1f();
}

void genbc1t()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1T, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)BC1T, 1);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T2, 0x800000u);
    emit_and(A64_T1, A64_T1, A64_T2, false);
    emit_cmp_w(A64_T1, A64_ZR);
    emit_cset(A64_T2, A64_COND_NE);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&branch_taken);
    emit_str_w_off(A64_T2, A64_T1, 0);

    gendelayslot();
    gentest();
}

void genbc1t_out()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1T_OUT, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)BC1T_OUT, 1);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T2, 0x800000u);
    emit_and(A64_T1, A64_T1, A64_T2, false);
    emit_cmp_w(A64_T1, A64_ZR);
    emit_cset(A64_T2, A64_COND_NE);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&branch_taken);
    emit_str_w_off(A64_T2, A64_T1, 0);

    gendelayslot();
    gentest_out();
}

void genbc1t_idle()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1T_IDLE, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)BC1T_IDLE, 1);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T2, 0x800000u);
    emit_and(A64_T1, A64_T1, A64_T2, false);
    emit_cmp_w(A64_T1, A64_ZR);
    emit_cset(A64_T2, A64_COND_NE);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&branch_taken);
    emit_str_w_off(A64_T2, A64_T1, 0);

    gentest_idle();

    genbc1t();
}

void genbc1fl()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1FL, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)BC1FL, 1);
    const int32_t from_slow = emit_b(0);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T2, 0x800000u);
    emit_and(A64_T1, A64_T1, A64_T2, false);
    emit_cmp_w(A64_T1, A64_ZR);
    emit_cset(A64_T2, A64_COND_EQ);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&branch_taken);
    emit_str_w_off(A64_T2, A64_T1, 0);

    free_all_registers();
    gentestl_impl(false);
    patch_branch(from_slow, (code_length - from_slow) / 4);
}

void genbc1fl_out()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1FL_OUT, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)BC1FL_OUT, 1);
    const int32_t from_slow = emit_b(0);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T2, 0x800000u);
    emit_and(A64_T1, A64_T1, A64_T2, false);
    emit_cmp_w(A64_T1, A64_ZR);
    emit_cset(A64_T2, A64_COND_EQ);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&branch_taken);
    emit_str_w_off(A64_T2, A64_T1, 0);

    free_all_registers();
    gentestl_out_impl(false);
    patch_branch(from_slow, (code_length - from_slow) / 4);
}

void genbc1fl_idle()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1FL_IDLE, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)BC1FL_IDLE, 1);
    const int32_t from_slow = emit_b(0);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T2, 0x800000u);
    emit_and(A64_T1, A64_T1, A64_T2, false);
    emit_cmp_w(A64_T1, A64_ZR);
    emit_cset(A64_T2, A64_COND_EQ);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&branch_taken);
    emit_str_w_off(A64_T2, A64_T1, 0);

    gentest_idle();

    genbc1fl();
    patch_branch(from_slow, (code_length - from_slow) / 4);
}

void genbc1tl()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1TL, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)BC1TL, 1);
    const int32_t from_slow = emit_b(0);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T2, 0x800000u);
    emit_and(A64_T1, A64_T1, A64_T2, false);
    emit_cmp_w(A64_T1, A64_ZR);
    emit_cset(A64_T2, A64_COND_NE);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&branch_taken);
    emit_str_w_off(A64_T2, A64_T1, 0);

    free_all_registers();
    gentestl_impl(false);
    patch_branch(from_slow, (code_length - from_slow) / 4);
}

void genbc1tl_out()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1TL_OUT, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)BC1TL_OUT, 1);
    const int32_t from_slow = emit_b(0);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T2, 0x800000u);
    emit_and(A64_T1, A64_T1, A64_T2, false);
    emit_cmp_w(A64_T1, A64_ZR);
    emit_cset(A64_T2, A64_COND_NE);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&branch_taken);
    emit_str_w_off(A64_T2, A64_T1, 0);

    free_all_registers();
    gentestl_out_impl(false);
    patch_branch(from_slow, (code_length - from_slow) / 4);
}

void genbc1tl_idle()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1TL_IDLE, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)BC1TL_IDLE, 1);
    const int32_t from_slow = emit_b(0);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&FCR31);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T2, 0x800000u);
    emit_and(A64_T1, A64_T1, A64_T2, false);
    emit_cmp_w(A64_T1, A64_ZR);
    emit_cset(A64_T2, A64_COND_NE);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&branch_taken);
    emit_str_w_off(A64_T2, A64_T1, 0);

    gentest_idle();

    genbc1tl();
    patch_branch(from_slow, (code_length - from_slow) / 4);
}
