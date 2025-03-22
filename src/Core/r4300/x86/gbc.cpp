/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <r4300/x86/assemble.h>
#include <r4300/ops.h>
#include <r4300/r4300.h>
#include <r4300/recomph.h>
#include <r4300/x86/regcache.h>

void genbc1f_test()
{
    test_m32_imm32((uint32_t*)&FCR31, 0x800000);
    jne_rj(12);
    mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
    jmp_imm_short(10); // 2
    mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
}

void genbc1f()
{
#ifdef INTERPRET_BC1F
    gencallinterp((uint32_t)BC1F, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BC1F, 1);
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
    gencallinterp((uint32_t)BC1F_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BC1F_OUT, 1);
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
    gencallinterp((uint32_t)BC1F_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BC1F_IDLE, 1);
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
    test_m32_imm32((uint32_t*)&FCR31, 0x800000);
    je_rj(12);
    mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
    jmp_imm_short(10); // 2
    mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
}

void genbc1t()
{
#ifdef INTERPRET_BC1T
    gencallinterp((uint32_t)BC1T, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BC1T, 1);
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
    gencallinterp((uint32_t)BC1T_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BC1T_OUT, 1);
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
    gencallinterp((uint32_t)BC1T_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BC1T_IDLE, 1);
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
    gencallinterp((uint32_t)BC1FL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BC1FL, 1);
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
    gencallinterp((uint32_t)BC1FL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BC1FL_OUT, 1);
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
    gencallinterp((uint32_t)BC1FL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BC1FL_IDLE, 1);
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
    gencallinterp((uint32_t)BC1TL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BC1TL, 1);
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
    gencallinterp((uint32_t)BC1TL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BC1TL_OUT, 1);
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
    gencallinterp((uint32_t)BC1TL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BC1TL_IDLE, 1);
        return;
    }

    gencheck_cop1_unusable();
    genbc1t_test();
    gentest_idle();
    genbc1tl();
#endif
}
