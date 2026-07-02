/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <Core.hpp>
#include <R4300/x86_64/Assemble.hpp>
#include <R4300/Ops.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/x86_64/RegCache.hpp>

void genbc1f_test()
{
    // Patched jumps: the old hardcoded offsets (jne 12, jmp 10) assumed a 10-byte
    // mov_m32_imm32; on x64 it is 17 bytes, so the jumps landed mid-instruction.
    test_m32_imm32((uint32_t *)&FCR31, 0x800000);
    jne_rj(0);
    int32_t j = code_length - 1;
    mov_m32_imm32((void *)(&branch_taken), (uintptr_t)1);
    jmp_imm_short(0);
    int32_t jend = code_length - 1;
    rj_patch(j);
    mov_m32_imm32((void *)(&branch_taken), (uintptr_t)0);
    rj_patch(jend);
}

void genbc1f()
{
#ifdef INTERPRET_BC1F
    gencallinterp((uintptr_t)BC1F, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1F, 1);
        return;
    }

    gencheck_cop1_unusable();
    genbc1f_test();
    gendelayslot();
    gentest();
#endif
}

void genbc1f_out()
{
#ifdef INTERPRET_BC1F_OUT
    gencallinterp((uintptr_t)BC1F_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1F_OUT, 1);
        return;
    }

    gencheck_cop1_unusable();
    genbc1f_test();
    gendelayslot();
    gentest_out();
#endif
}

void genbc1f_idle()
{
#ifdef INTERPRET_BC1F_IDLE
    gencallinterp((uintptr_t)BC1F_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1F_IDLE, 1);
        return;
    }

    gencheck_cop1_unusable();
    genbc1f_test();
    gentest_idle();
    genbc1f();
#endif
}

void genbc1t_test()
{
    // Patched jumps (see genbc1f_test): x64 mov_m32_imm32 is 17 bytes, not 10.
    test_m32_imm32((uint32_t *)&FCR31, 0x800000);
    je_rj(0);
    int32_t j = code_length - 1;
    mov_m32_imm32((void *)(&branch_taken), (uintptr_t)1);
    jmp_imm_short(0);
    int32_t jend = code_length - 1;
    rj_patch(j);
    mov_m32_imm32((void *)(&branch_taken), (uintptr_t)0);
    rj_patch(jend);
}

void genbc1t()
{
#ifdef INTERPRET_BC1T
    gencallinterp((uintptr_t)BC1T, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1T, 1);
        return;
    }

    gencheck_cop1_unusable();
    genbc1t_test();
    gendelayslot();
    gentest();
#endif
}

void genbc1t_out()
{
#ifdef INTERPRET_BC1T_OUT
    gencallinterp((uintptr_t)BC1T_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1T_OUT, 1);
        return;
    }

    gencheck_cop1_unusable();
    genbc1t_test();
    gendelayslot();
    gentest_out();
#endif
}

void genbc1t_idle()
{
#ifdef INTERPRET_BC1T_IDLE
    gencallinterp((uintptr_t)BC1T_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1T_IDLE, 1);
        return;
    }

    gencheck_cop1_unusable();
    genbc1t_test();
    gentest_idle();
    genbc1t();
#endif
}

void genbc1fl()
{
#ifdef INTERPRET_BC1FL
    gencallinterp((uintptr_t)BC1FL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1FL, 1);
        return;
    }

    gencheck_cop1_unusable();
    genbc1f_test();
    free_all_registers();
    gentestl();
#endif
}

void genbc1fl_out()
{
#ifdef INTERPRET_BC1FL_OUT
    gencallinterp((uintptr_t)BC1FL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1FL_OUT, 1);
        return;
    }

    gencheck_cop1_unusable();
    genbc1f_test();
    free_all_registers();
    gentestl_out();
#endif
}

void genbc1fl_idle()
{
#ifdef INTERPRET_BC1FL_IDLE
    gencallinterp((uintptr_t)BC1FL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1FL_IDLE, 1);
        return;
    }

    gencheck_cop1_unusable();
    genbc1f_test();
    gentest_idle();
    genbc1fl();
#endif
}

void genbc1tl()
{
#ifdef INTERPRET_BC1TL
    gencallinterp((uintptr_t)BC1TL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1TL, 1);
        return;
    }

    gencheck_cop1_unusable();
    genbc1t_test();
    free_all_registers();
    gentestl();
#endif
}

void genbc1tl_out()
{
#ifdef INTERPRET_BC1TL_OUT
    gencallinterp((uintptr_t)BC1TL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1TL_OUT, 1);
        return;
    }

    gencheck_cop1_unusable();
    genbc1t_test();
    free_all_registers();
    gentestl_out();
#endif
}

void genbc1tl_idle()
{
#ifdef INTERPRET_BC1TL_IDLE
    gencallinterp((uintptr_t)BC1TL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BC1TL_IDLE, 1);
        return;
    }

    gencheck_cop1_unusable();
    genbc1t_test();
    gentest_idle();
    genbc1tl();
#endif
}
