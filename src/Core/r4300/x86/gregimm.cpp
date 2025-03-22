/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <r4300/ops.h>
#include <r4300/r4300.h>
#include <r4300/recomp.h>
#include <r4300/recomph.h>
#include <r4300/x86/assemble.h>
#include <r4300/x86/regcache.h>

void genbltz_test()
{
    int32_t rs_64bit = is64((uint32_t*)dst->f.i.rs);

    if (!rs_64bit)
    {
        int32_t rs = allocate_register((uint32_t*)dst->f.i.rs);

        cmp_reg32_imm32(rs, 0);
        jge_rj(12);
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
    }
    else if (rs_64bit == -1)
    {
        cmp_m32_imm32(((uint32_t*)dst->f.i.rs) + 1, 0);
        jge_rj(12);
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
    }
    else
    {
        int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);

        cmp_reg32_imm32(rs2, 0);
        jge_rj(12);
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
    }
}

void genbltz()
{
#ifdef INTERPRET_BLTZ
    gencallinterp((uint32_t)BLTZ, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLTZ, 1);
        return;
    }

    genbltz_test();
    gendelayslot();
    gentest();
#endif
}

void genbltz_out()
{
#ifdef INTERPRET_BLTZ_OUT
    gencallinterp((uint32_t)BLTZ_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLTZ_OUT, 1);
        return;
    }

    genbltz_test();
    gendelayslot();
    gentest_out();
#endif
}

void genbltz_idle()
{
#ifdef INTERPRET_BLTZ_IDLE
    gencallinterp((uint32_t)BLTZ_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLTZ_IDLE, 1);
        return;
    }

    genbltz_test();
    gentest_idle();
    genbltz();
#endif
}

void genbgez_test()
{
    int32_t rs_64bit = is64((uint32_t*)dst->f.i.rs);

    if (!rs_64bit)
    {
        int32_t rs = allocate_register((uint32_t*)dst->f.i.rs);

        cmp_reg32_imm32(rs, 0);
        jl_rj(12);
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
    }
    else if (rs_64bit == -1)
    {
        cmp_m32_imm32(((uint32_t*)dst->f.i.rs) + 1, 0);
        jl_rj(12);
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
    }
    else
    {
        int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);

        cmp_reg32_imm32(rs2, 0);
        jl_rj(12);
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
    }
}

void genbgez()
{
#ifdef INTERPRET_BGEZ
    gencallinterp((uint32_t)BGEZ, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGEZ, 1);
        return;
    }

    genbgez_test();
    gendelayslot();
    gentest();
#endif
}

void genbgez_out()
{
#ifdef INTERPRET_BGEZ_OUT
    gencallinterp((uint32_t)BGEZ_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGEZ_OUT, 1);
        return;
    }

    genbgez_test();
    gendelayslot();
    gentest_out();
#endif
}

void genbgez_idle()
{
#ifdef INTERPRET_BGEZ_IDLE
    gencallinterp((uint32_t)BGEZ_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGEZ_IDLE, 1);
        return;
    }

    genbgez_test();
    gentest_idle();
    genbgez();
#endif
}

void genbltzl()
{
#ifdef INTERPRET_BLTZL
    gencallinterp((uint32_t)BLTZL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLTZL, 1);
        return;
    }

    genbltz_test();
    free_all_registers();
    gentestl();
#endif
}

void genbltzl_out()
{
#ifdef INTERPRET_BLTZL_OUT
    gencallinterp((uint32_t)BLTZL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLTZL_OUT, 1);
        return;
    }

    genbltz_test();
    free_all_registers();
    gentestl_out();
#endif
}

void genbltzl_idle()
{
#ifdef INTERPRET_BLTZL_IDLE
    gencallinterp((uint32_t)BLTZL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLTZL_IDLE, 1);
        return;
    }

    genbltz_test();
    gentest_idle();
    genbltzl();
#endif
}

void genbgezl()
{
#ifdef INTERPRET_BGEZL
    gencallinterp((uint32_t)BGEZL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGEZL, 1);
        return;
    }

    genbgez_test();
    free_all_registers();
    gentestl();
#endif
}

void genbgezl_out()
{
#ifdef INTERPRET_BGEZL_OUT
    gencallinterp((uint32_t)BGEZL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGEZL_OUT, 1);
        return;
    }

    genbgez_test();
    free_all_registers();
    gentestl_out();
#endif
}

void genbgezl_idle()
{
#ifdef INTERPRET_BGEZL_IDLE
    gencallinterp((uint32_t)BGEZL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGEZL_IDLE, 1);
        return;
    }

    genbgez_test();
    gentest_idle();
    genbgezl();
#endif
}

void genbranchlink()
{
    int32_t r31_64bit = is64((uint32_t*)&reg[31]);

    if (!r31_64bit)
    {
        int32_t r31 = allocate_register_w((uint32_t*)&reg[31]);

        mov_reg32_imm32(r31, dst->addr + 8);
    }
    else if (r31_64bit == -1)
    {
        mov_m32_imm32((uint32_t*)&reg[31], dst->addr + 8);
        if (dst->addr & 0x80000000)
            mov_m32_imm32(((uint32_t*)&reg[31]) + 1, 0xFFFFFFFF);
        else
            mov_m32_imm32(((uint32_t*)&reg[31]) + 1, 0);
    }
    else
    {
        int32_t r311 = allocate_64_register1_w((uint32_t*)&reg[31]);
        int32_t r312 = allocate_64_register2_w((uint32_t*)&reg[31]);

        mov_reg32_imm32(r311, dst->addr + 8);
        if (dst->addr & 0x80000000)
            mov_reg32_imm32(r312, 0xFFFFFFFF);
        else
            mov_reg32_imm32(r312, 0);
    }
}

void genbltzal()
{
#ifdef INTERPRET_BLTZAL
    gencallinterp((uint32_t)BLTZAL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLTZAL, 1);
        return;
    }

    genbltz_test();
    genbranchlink();
    gendelayslot();
    gentest();
#endif
}

void genbltzal_out()
{
#ifdef INTERPRET_BLTZAL_OUT
    gencallinterp((uint32_t)BLTZAL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLTZAL_OUT, 1);
        return;
    }

    genbltz_test();
    genbranchlink();
    gendelayslot();
    gentest_out();
#endif
}

void genbltzal_idle()
{
#ifdef INTERPRET_BLTZAL_IDLE
    gencallinterp((uint32_t)BLTZAL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLTZAL_IDLE, 1);
        return;
    }

    genbltz_test();
    genbranchlink();
    gentest_idle();
    genbltzal();
#endif
}

void genbgezal()
{
#ifdef INTERPRET_BGEZAL
    gencallinterp((uint32_t)BGEZAL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGEZAL, 1);
        return;
    }

    genbgez_test();
    genbranchlink();
    gendelayslot();
    gentest();
#endif
}

void genbgezal_out()
{
#ifdef INTERPRET_BGEZAL_OUT
    gencallinterp((uint32_t)BGEZAL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGEZAL_OUT, 1);
        return;
    }

    genbgez_test();
    genbranchlink();
    gendelayslot();
    gentest_out();
#endif
}

void genbgezal_idle()
{
#ifdef INTERPRET_BGEZAL_IDLE
    gencallinterp((uint32_t)BGEZAL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGEZAL_IDLE, 1);
        return;
    }

    genbgez_test();
    genbranchlink();
    gentest_idle();
    genbgezal();
#endif
}

void genbltzall()
{
#ifdef INTERPRET_BLTZALL
    gencallinterp((uint32_t)BLTZALL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLTZALL, 1);
        return;
    }

    genbltz_test();
    genbranchlink();
    free_all_registers();
    gentestl();
#endif
}

void genbltzall_out()
{
#ifdef INTERPRET_BLTZALL_OUT
    gencallinterp((uint32_t)BLTZALL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLTZALL_OUT, 1);
        return;
    }

    genbltz_test();
    genbranchlink();
    free_all_registers();
    gentestl_out();
#endif
}

void genbltzall_idle()
{
#ifdef INTERPRET_BLTZALL_IDLE
    gencallinterp((uint32_t)BLTZALL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLTZALL_IDLE, 1);
        return;
    }

    genbltz_test();
    genbranchlink();
    gentest_idle();
    genbltzall();
#endif
}

void genbgezall()
{
#ifdef INTERPRET_BGEZALL
    gencallinterp((uint32_t)BGEZALL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGEZALL, 1);
        return;
    }

    genbgez_test();
    genbranchlink();
    free_all_registers();
    gentestl();
#endif
}

void genbgezall_out()
{
#ifdef INTERPRET_BGEZALL_OUT
    gencallinterp((uint32_t)BGEZALL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGEZALL_OUT, 1);
        return;
    }

    genbgez_test();
    genbranchlink();
    free_all_registers();
    gentestl_out();
#endif
}

void genbgezall_idle()
{
#ifdef INTERPRET_BGEZALL_IDLE
    gencallinterp((uint32_t)BGEZALL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC &&
         (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGEZALL_IDLE, 1);
        return;
    }

    genbgez_test();
    genbranchlink();
    gentest_idle();
    genbgezall();
#endif
}
