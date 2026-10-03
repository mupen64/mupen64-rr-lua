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

void genbltz()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLTZ, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LT);
    gendelayslot();
    gentest();
}

void genbltz_out()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLTZ_OUT, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LT);
    gendelayslot();
    gentest_out();
}

void genbltz_idle()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLTZ_IDLE, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LT);
    gentest_idle();

    genbltz();
}

void genbgez()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGEZ, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GE);
    gendelayslot();
    gentest();
}

void genbgez_out()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGEZ_OUT, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GE);
    gendelayslot();
    gentest_out();
}

void genbgez_idle()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGEZ_IDLE, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GE);
    gentest_idle();

    genbgez();
}

void genbltzl()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLTZL, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LT);
    free_all_registers();
    gentestl_impl(false);
}

void genbltzl_out()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLTZL_OUT, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LT);
    free_all_registers();
    gentestl_out_impl(false);
}

void genbltzl_idle()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLTZL_IDLE, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LT);
    gentest_idle();

    genbltzl();
}

void genbgezl()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGEZL, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GE);
    free_all_registers();
    gentestl_impl(false);
}

void genbgezl_out()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGEZL_OUT, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GE);
    free_all_registers();
    gentestl_out_impl(false);
}

void genbgezl_idle()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGEZL_IDLE, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GE);
    gentest_idle();

    genbgezl();
}

void genbltzal()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLTZAL, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LT);
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 8));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    gendelayslot();
    gentest();
}

void genbltzal_out()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLTZAL_OUT, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LT);
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 8));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    gendelayslot();
    gentest_out();
}

void genbltzal_idle()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLTZAL_IDLE, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LT);
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 8));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    gentest_idle();

    genbltzal();
}

void genbgezal()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGEZAL, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GE);
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 8));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    gendelayslot();
    gentest();
}

void genbgezal_out()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGEZAL_OUT, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GE);
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 8));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    gendelayslot();
    gentest_out();
}

void genbgezal_idle()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGEZAL_IDLE, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GE);
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 8));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    gentest_idle();

    genbgezal();
}

void genbltzall()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLTZALL, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LT);
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 8));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    gentestl_impl(false);
}

void genbltzall_out()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLTZALL_OUT, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LT);
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 8));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    gentestl_out_impl(false);
}

void genbltzall_idle()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLTZALL_IDLE, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LT);
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 8));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    gentest_idle();

    genbltzall();
}

void genbgezall()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGEZALL, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GE);
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 8));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    gentestl_impl(false);
}

void genbgezall_out()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGEZALL_OUT, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GE);
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 8));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    gentestl_out_impl(false);
}

void genbgezall_idle()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGEZALL_IDLE, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GE);
    free_all_registers();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 8));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    gentest_idle();

    genbgezall();
}
