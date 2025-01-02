/**
 * Mupen64 - gbc.c
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
 **/

#include <shared/Config.hpp>
#include <stdio.h>
#include "../ops.h"
#include "../r4300.h"
#include "../recomph.h"
#include "assemble.h"
#include "interpret.h"

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
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
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
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
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
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
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
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
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
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
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
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
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
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
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
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
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
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
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
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
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
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
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
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
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
