/**
 * Mupen64 - gr4300.c
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
#include "../../memory/memory.h"
#include "../exception.h"
#include "../interrupt.h"
#include "../macros.h"
#include "../ops.h"
#include "../r4300.h"
#include "../recomph.h"
#include "assemble.h"
#include "interpret.h"
#include "regcache.h"

extern uint32_t src; // recomp.c

precomp_instr fake_instr;
static int32_t eax, ebx, ecx, edx, esp, ebp, esi, edi;

int32_t branch_taken;

void gennotcompiled()
{
    free_all_registers();
    simplify_access();

    if (dst->addr == 0xa4000040)
    {
        mov_m32_reg32((uint32_t*)(&return_address), ESP);
        sub_m32_imm32((uint32_t*)(&return_address), 4);
    }
    mov_m32_imm32((uint32_t*)(&PC), (uint32_t)(dst));
    mov_reg32_imm32(EAX, (uint32_t)NOTCOMPILED);
    call_reg32(EAX);
}

void genlink_subblock()
{
    free_all_registers();
    jmp(dst->addr + 4);
}

void gendebug()
{
    // free_all_registers();
    mov_m32_reg32((uint32_t*)&eax, EAX);
    mov_m32_reg32((uint32_t*)&ebx, EBX);
    mov_m32_reg32((uint32_t*)&ecx, ECX);
    mov_m32_reg32((uint32_t*)&edx, EDX);
    mov_m32_reg32((uint32_t*)&esp, ESP);
    mov_m32_reg32((uint32_t*)&ebp, EBP);
    mov_m32_reg32((uint32_t*)&esi, ESI);
    mov_m32_reg32((uint32_t*)&edi, EDI);

    mov_m32_imm32((uint32_t*)(&PC), (uint32_t)(dst));
    mov_m32_imm32((uint32_t*)(&vr_op), (uint32_t)(src));
    mov_reg32_imm32(EAX, (uint32_t)debug);
    call_reg32(EAX);

    mov_reg32_m32(EAX, (uint32_t*)&eax);
    mov_reg32_m32(EBX, (uint32_t*)&ebx);
    mov_reg32_m32(ECX, (uint32_t*)&ecx);
    mov_reg32_m32(EDX, (uint32_t*)&edx);
    mov_reg32_m32(ESP, (uint32_t*)&esp);
    mov_reg32_m32(EBP, (uint32_t*)&ebp);
    mov_reg32_m32(ESI, (uint32_t*)&esi);
    mov_reg32_m32(EDI, (uint32_t*)&edi);
}

void gencallinterp(uint32_t addr, int32_t jump)
{
    free_all_registers();
    simplify_access();
    if (jump)
        mov_m32_imm32((uint32_t*)(&dyna_interp), 1);
    mov_m32_imm32((uint32_t*)(&PC), (uint32_t)(dst));
    mov_reg32_imm32(EAX, addr);
    call_reg32(EAX);
    if (jump)
    {
        mov_m32_imm32((uint32_t*)(&dyna_interp), 0);
        mov_reg32_imm32(EAX, (uint32_t)dyna_jump);
        call_reg32(EAX);
    }
}

void genupdate_count(uint32_t addr)
{
#ifndef COMPARE_CORE
#ifndef DBG
    mov_reg32_imm32(EAX, addr);
    sub_reg32_m32(EAX, (uint32_t*)(&last_addr));
    shr_reg32_imm8(EAX, 1);
    add_m32_reg32((uint32_t*)(&core_Count), EAX);
#else
    mov_m32_imm32((uint32_t*)(&PC), (uint32_t)(dst + 1));
    mov_reg32_imm32(EAX, (uint32_t)update_count);
    call_reg32(EAX);
#endif
#else
    mov_m32_imm32((uint32_t*)(&PC), (uint32_t)(dst + 1));
    mov_reg32_imm32(EAX, (uint32_t)update_count);
    call_reg32(EAX);
#endif
}

void gendelayslot()
{
    mov_m32_imm32((void*)(&delay_slot), 1);
    recompile_opcode();

    free_all_registers();
    genupdate_count(dst->addr + 4);

    mov_m32_imm32((void*)(&delay_slot), 0);
}

void genni()
{
#ifdef EMU64_DEBUG
    gencallinterp((uint32_t)NI, 0);
#endif
}

void genreserved()
{
#ifdef EMU64_DEBUG
    gencallinterp((uint32_t)RESERVED, 0);
#endif
}

void genfin_block() { gencallinterp((uint32_t)FIN_BLOCK, 0); }

void gencheck_interrupt(uint32_t instr_structure)
{
    mov_eax_memoffs32((void*)(&next_interrupt));
    cmp_reg32_m32(EAX, (void*)&core_Count);
    ja_rj(17);
    mov_m32_imm32((uint32_t*)(&PC), instr_structure); // 10
    mov_reg32_imm32(EAX, (uint32_t)gen_interrupt); // 5
    call_reg32(EAX); // 2
}

void gencheck_interrupt_out(uint32_t addr)
{
    mov_eax_memoffs32((void*)(&next_interrupt));
    cmp_reg32_m32(EAX, (void*)&core_Count);
    ja_rj(27);
    mov_m32_imm32((uint32_t*)(&fake_instr.addr), addr);
    mov_m32_imm32((uint32_t*)(&PC), (uint32_t)(&fake_instr));
    mov_reg32_imm32(EAX, (uint32_t)gen_interrupt);
    call_reg32(EAX);
}

void gencheck_interrupt_reg() // addr is in EAX
{
    mov_reg32_m32(EBX, (void*)&next_interrupt);
    cmp_reg32_m32(EBX, (void*)&core_Count);
    ja_rj(22);
    mov_memoffs32_eax((uint32_t*)(&fake_instr.addr)); // 5
    mov_m32_imm32((uint32_t*)(&PC), (uint32_t)(&fake_instr)); // 10
    mov_reg32_imm32(EAX, (uint32_t)gen_interrupt); // 5
    call_reg32(EAX); // 2
}

void gennop() {}

void genj()
{
#ifdef INTERPRET_J
    gencallinterp((uint32_t)J, 1);
#else
    uint32_t naddr;

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)J, 1);
        return;
    }

    gendelayslot();
    naddr = ((dst - 1)->f.j.inst_index << 2) | (dst->addr & 0xF0000000);

    mov_m32_imm32((void*)(&last_addr), naddr);
    gencheck_interrupt((uint32_t)&actual->block[(naddr - actual->start) / 4]);
    jmp(naddr);
#endif
}

void genj_out()
{
#ifdef INTERPRET_J_OUT
    gencallinterp((uint32_t)J_OUT, 1);
#else
    uint32_t naddr;

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)J_OUT, 1);
        return;
    }

    gendelayslot();
    naddr = ((dst - 1)->f.j.inst_index << 2) | (dst->addr & 0xF0000000);

    mov_m32_imm32((void*)(&last_addr), naddr);
    gencheck_interrupt_out(naddr);
    mov_m32_imm32(&jump_to_address, naddr);
    mov_m32_imm32((uint32_t*)(&PC), (uint32_t)(dst + 1));
    mov_reg32_imm32(EAX, (uint32_t)jump_to_func);
    call_reg32(EAX);
#endif
}

void genj_idle()
{
#ifdef INTERPRET_J_IDLE
    gencallinterp((uint32_t)J_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)J_IDLE, 1);
        return;
    }

    mov_eax_memoffs32((uint32_t*)(&next_interrupt));
    sub_reg32_m32(EAX, (uint32_t*)(&core_Count));
    cmp_reg32_imm8(EAX, 3);
    jbe_rj(11);

    and_eax_imm32(0xFFFFFFFC);
    add_m32_reg32((uint32_t*)(&core_Count), EAX);

    genj();
#endif
}

void genjal()
{
#ifdef INTERPRET_JAL
    gencallinterp((uint32_t)JAL, 1);
#else
    uint32_t naddr;

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)JAL, 1);
        return;
    }

    gendelayslot();

    mov_m32_imm32((uint32_t*)(reg + 31), dst->addr + 4);
    if (((dst->addr + 4) & 0x80000000))
        mov_m32_imm32((uint32_t*)(&reg[31]) + 1, 0xFFFFFFFF);
    else
        mov_m32_imm32((uint32_t*)(&reg[31]) + 1, 0);

    naddr = ((dst - 1)->f.j.inst_index << 2) | (dst->addr & 0xF0000000);

    mov_m32_imm32((void*)(&last_addr), naddr);
    gencheck_interrupt((uint32_t)&actual->block[(naddr - actual->start) / 4]);
    jmp(naddr);
#endif
}

void genjal_out()
{
#ifdef INTERPRET_JAL_OUT
    gencallinterp((uint32_t)JAL_OUT, 1);
#else
    uint32_t naddr;

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)JAL_OUT, 1);
        return;
    }

    gendelayslot();

    mov_m32_imm32((uint32_t*)(reg + 31), dst->addr + 4);
    if (((dst->addr + 4) & 0x80000000))
        mov_m32_imm32((uint32_t*)(&reg[31]) + 1, 0xFFFFFFFF);
    else
        mov_m32_imm32((uint32_t*)(&reg[31]) + 1, 0);

    naddr = ((dst - 1)->f.j.inst_index << 2) | (dst->addr & 0xF0000000);

    mov_m32_imm32((void*)(&last_addr), naddr);
    gencheck_interrupt_out(naddr);
    mov_m32_imm32(&jump_to_address, naddr);
    mov_m32_imm32((uint32_t*)(&PC), (uint32_t)(dst + 1));
    mov_reg32_imm32(EAX, (uint32_t)jump_to_func);
    call_reg32(EAX);
#endif
}

void genjal_idle()
{
#ifdef INTERPRET_JAL_IDLE
    gencallinterp((uint32_t)JAL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)JAL_IDLE, 1);
        return;
    }

    mov_eax_memoffs32((uint32_t*)(&next_interrupt));
    sub_reg32_m32(EAX, (uint32_t*)(&core_Count));
    cmp_reg32_imm8(EAX, 3);
    jbe_rj(11);

    and_eax_imm32(0xFFFFFFFC);
    add_m32_reg32((uint32_t*)(&core_Count), EAX);

    genjal();
#endif
}

void genbeq_test()
{
    int32_t rs_64bit = is64((uint32_t*)dst->f.i.rs);
    int32_t rt_64bit = is64((uint32_t*)dst->f.i.rt);

    if (!rs_64bit && !rt_64bit)
    {
        int32_t rs = allocate_register((uint32_t*)dst->f.i.rs);
        int32_t rt = allocate_register((uint32_t*)dst->f.i.rt);

        cmp_reg32_reg32(rs, rt);
        jne_rj(12);
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
    }
    else if (rs_64bit == -1)
    {
        int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.i.rt);
        int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.i.rt);

        cmp_reg32_m32(rt1, (uint32_t*)dst->f.i.rs);
        jne_rj(20);
        cmp_reg32_m32(rt2, ((uint32_t*)dst->f.i.rs) + 1); // 6
        jne_rj(12); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
    }
    else if (rt_64bit == -1)
    {
        int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
        int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);

        cmp_reg32_m32(rs1, (uint32_t*)dst->f.i.rt);
        jne_rj(20);
        cmp_reg32_m32(rs2, ((uint32_t*)dst->f.i.rt) + 1); // 6
        jne_rj(12); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
    }
    else
    {
        int32_t rs1, rs2, rt1, rt2;
        if (!rs_64bit)
        {
            rt1 = allocate_64_register1((uint32_t*)dst->f.i.rt);
            rt2 = allocate_64_register2((uint32_t*)dst->f.i.rt);
            rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
            rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);
        }
        else
        {
            rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
            rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);
            rt1 = allocate_64_register1((uint32_t*)dst->f.i.rt);
            rt2 = allocate_64_register2((uint32_t*)dst->f.i.rt);
        }
        cmp_reg32_reg32(rs1, rt1);
        jne_rj(16);
        cmp_reg32_reg32(rs2, rt2); // 2
        jne_rj(12); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
    }
}

void gentest()
{
    uint32_t temp, temp2;

    cmp_m32_imm32((uint32_t*)(&branch_taken), 0);
    je_near_rj(0);
    temp = code_length;
    mov_m32_imm32((void*)(&last_addr), dst->addr + (dst - 1)->f.i.immediate * 4);
    gencheck_interrupt((uint32_t)(dst + (dst - 1)->f.i.immediate));
    jmp(dst->addr + (dst - 1)->f.i.immediate * 4);

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;
    mov_m32_imm32((void*)(&last_addr), dst->addr + 4);
    gencheck_interrupt((uint32_t)(dst + 1));
    jmp(dst->addr + 4);
}

void genbeq()
{
#ifdef INTERPRET_BEQ
    gencallinterp((uint32_t)BEQ, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BEQ, 1);
        return;
    }

    genbeq_test();
    gendelayslot();
    gentest();
#endif
}

void gentest_out()
{
    uint32_t temp, temp2;

    cmp_m32_imm32((uint32_t*)(&branch_taken), 0);
    je_near_rj(0);
    temp = code_length;
    mov_m32_imm32((void*)(&last_addr), dst->addr + (dst - 1)->f.i.immediate * 4);
    gencheck_interrupt_out(dst->addr + (dst - 1)->f.i.immediate * 4);
    mov_m32_imm32(&jump_to_address, dst->addr + (dst - 1)->f.i.immediate * 4);
    mov_m32_imm32((uint32_t*)(&PC), (uint32_t)(dst + 1));
    mov_reg32_imm32(EAX, (uint32_t)jump_to_func);
    call_reg32(EAX);

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;
    mov_m32_imm32((void*)(&last_addr), dst->addr + 4);
    gencheck_interrupt((uint32_t)(dst + 1));
    jmp(dst->addr + 4);
}

void genbeq_out()
{
#ifdef INTERPRET_BEQ_OUT
    gencallinterp((uint32_t)BEQ_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BEQ_OUT, 1);
        return;
    }

    genbeq_test();
    gendelayslot();
    gentest_out();
#endif
}

void gentest_idle()
{
    uint32_t temp, temp2;
    int32_t reg;

    reg = lru_register();
    free_register(reg);

    cmp_m32_imm32((uint32_t*)(&branch_taken), 0);
    je_near_rj(0);
    temp = code_length;

    mov_reg32_m32(reg, (uint32_t*)(&next_interrupt));
    sub_reg32_m32(reg, (uint32_t*)(&core_Count));
    cmp_reg32_imm8(reg, 3);
    jbe_rj(12);

    and_reg32_imm32(reg, 0xFFFFFFFC); // 6
    add_m32_reg32((uint32_t*)(&core_Count), reg); // 6

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;
}

void genbeq_idle()
{
#ifdef INTERPRET_BEQ_IDLE
    gencallinterp((uint32_t)BEQ_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BEQ_IDLE, 1);
        return;
    }

    genbeq_test();
    gentest_idle();
    genbeq();
#endif
}

void genbne_test()
{
    int32_t rs_64bit = is64((uint32_t*)dst->f.i.rs);
    int32_t rt_64bit = is64((uint32_t*)dst->f.i.rt);

    if (!rs_64bit && !rt_64bit)
    {
        int32_t rs = allocate_register((uint32_t*)dst->f.i.rs);
        int32_t rt = allocate_register((uint32_t*)dst->f.i.rt);

        cmp_reg32_reg32(rs, rt);
        je_rj(12);
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
    }
    else if (rs_64bit == -1)
    {
        int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.i.rt);
        int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.i.rt);

        cmp_reg32_m32(rt1, (uint32_t*)dst->f.i.rs);
        jne_rj(20);
        cmp_reg32_m32(rt2, ((uint32_t*)dst->f.i.rs) + 1); // 6
        jne_rj(12); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
    }
    else if (rt_64bit == -1)
    {
        int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
        int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);

        cmp_reg32_m32(rs1, (uint32_t*)dst->f.i.rt);
        jne_rj(20);
        cmp_reg32_m32(rs2, ((uint32_t*)dst->f.i.rt) + 1); // 6
        jne_rj(12); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
    }
    else
    {
        int32_t rs1, rs2, rt1, rt2;
        if (!rs_64bit)
        {
            rt1 = allocate_64_register1((uint32_t*)dst->f.i.rt);
            rt2 = allocate_64_register2((uint32_t*)dst->f.i.rt);
            rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
            rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);
        }
        else
        {
            rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
            rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);
            rt1 = allocate_64_register1((uint32_t*)dst->f.i.rt);
            rt2 = allocate_64_register2((uint32_t*)dst->f.i.rt);
        }
        cmp_reg32_reg32(rs1, rt1);
        jne_rj(16);
        cmp_reg32_reg32(rs2, rt2); // 2
        jne_rj(12); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
    }
}

void genbne()
{
#ifdef INTERPRET_BNE
    gencallinterp((uint32_t)BNE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BNE, 1);
        return;
    }

    genbne_test();
    gendelayslot();
    gentest();
#endif
}

void genbne_out()
{
#ifdef INTERPRET_BNE_OUT
    gencallinterp((uint32_t)BNE_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BNE_OUT, 1);
        return;
    }

    genbne_test();
    gendelayslot();
    gentest_out();
#endif
}

void genbne_idle()
{
#ifdef INTERPRET_BNE_IDLE
    gencallinterp((uint32_t)BNE_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BNE_IDLE, 1);
        return;
    }

    genbne_test();
    gentest_idle();
    genbne();
#endif
}

void genblez_test()
{
    int32_t rs_64bit = is64((uint32_t*)dst->f.i.rs);

    if (!rs_64bit)
    {
        int32_t rs = allocate_register((uint32_t*)dst->f.i.rs);

        cmp_reg32_imm32(rs, 0);
        jg_rj(12);
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
    }
    else if (rs_64bit == -1)
    {
        cmp_m32_imm32(((uint32_t*)dst->f.i.rs) + 1, 0);
        jg_rj(14);
        jne_rj(24); // 2
        cmp_m32_imm32((uint32_t*)dst->f.i.rs, 0); // 10
        je_rj(12); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
    }
    else
    {
        int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
        int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);

        cmp_reg32_imm32(rs2, 0);
        jg_rj(10);
        jne_rj(20); // 2
        cmp_reg32_imm32(rs1, 0); // 6
        je_rj(12); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
    }
}

void genblez()
{
#ifdef INTERPRET_BLEZ
    gencallinterp((uint32_t)BLEZ, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLEZ, 1);
        return;
    }

    genblez_test();
    gendelayslot();
    gentest();
#endif
}

void genblez_out()
{
#ifdef INTERPRET_BLEZ_OUT
    gencallinterp((uint32_t)BLEZ_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLEZ_OUT, 1);
        return;
    }

    genblez_test();
    gendelayslot();
    gentest_out();
#endif
}

void genblez_idle()
{
#ifdef INTERPRET_BLEZ_IDLE
    gencallinterp((uint32_t)BLEZ_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLEZ_IDLE, 1);
        return;
    }

    genblez_test();
    gentest_idle();
    genblez();
#endif
}

void genbgtz_test()
{
    int32_t rs_64bit = is64((uint32_t*)dst->f.i.rs);

    if (!rs_64bit)
    {
        int32_t rs = allocate_register((uint32_t*)dst->f.i.rs);

        cmp_reg32_imm32(rs, 0);
        jle_rj(12);
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
    }
    else if (rs_64bit == -1)
    {
        cmp_m32_imm32(((uint32_t*)dst->f.i.rs) + 1, 0);
        jl_rj(14);
        jne_rj(24); // 2
        cmp_m32_imm32((uint32_t*)dst->f.i.rs, 0); // 10
        jne_rj(12); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
    }
    else
    {
        int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
        int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);

        cmp_reg32_imm32(rs2, 0);
        jl_rj(10);
        jne_rj(20); // 2
        cmp_reg32_imm32(rs1, 0); // 6
        jne_rj(12); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
        jmp_imm_short(10); // 2
        mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
    }
}

void genbgtz()
{
#ifdef INTERPRET_BGTZ
    gencallinterp((uint32_t)BGTZ, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGTZ, 1);
        return;
    }

    genbgtz_test();
    gendelayslot();
    gentest();
#endif
}

void genbgtz_out()
{
#ifdef INTERPRET_BGTZ_OUT
    gencallinterp((uint32_t)BGTZ_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGTZ_OUT, 1);
        return;
    }

    genbgtz_test();
    gendelayslot();
    gentest_out();
#endif
}

void genbgtz_idle()
{
#ifdef INTERPRET_BGTZ_IDLE
    gencallinterp((uint32_t)BGTZ_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGTZ_IDLE, 1);
        return;
    }

    genbgtz_test();
    gentest_idle();
    genbgtz();
#endif
}

void genaddi()
{
#ifdef INTERPRET_ADDI
    gencallinterp((uint32_t)ADDI, 0);
#else
    int32_t rs = allocate_register((uint32_t*)dst->f.i.rs);
    int32_t rt = allocate_register_w((uint32_t*)dst->f.i.rt);

    mov_reg32_reg32(rt, rs);
    add_reg32_imm32(rt, (int32_t)dst->f.i.immediate);
#endif
}

void genaddiu()
{
#ifdef INTERPRET_ADDIU
    gencallinterp((uint32_t)ADDIU, 0);
#else
    int32_t rs = allocate_register((uint32_t*)dst->f.i.rs);
    int32_t rt = allocate_register_w((uint32_t*)dst->f.i.rt);

    mov_reg32_reg32(rt, rs);
    add_reg32_imm32(rt, (int32_t)dst->f.i.immediate);
#endif
}

void genslti()
{
#ifdef INTERPRET_SLTI
    gencallinterp((uint32_t)SLTI, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);
    int32_t rt = allocate_register_w((uint32_t*)dst->f.i.rt);
    int64_t imm = (int64_t)dst->f.i.immediate;

    cmp_reg32_imm32(rs2, (uint32_t)(imm >> 32));
    jl_rj(17);
    jne_rj(8); // 2
    cmp_reg32_imm32(rs1, (uint32_t)imm); // 6
    jl_rj(7); // 2
    mov_reg32_imm32(rt, 0); // 5
    jmp_imm_short(5); // 2
    mov_reg32_imm32(rt, 1); // 5
#endif
}

void gensltiu()
{
#ifdef INTERPRET_SLTIU
    gencallinterp((uint32_t)SLTIU, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);
    int32_t rt = allocate_register_w((uint32_t*)dst->f.i.rt);
    int64_t imm = (int64_t)dst->f.i.immediate;

    cmp_reg32_imm32(rs2, (uint32_t)(imm >> 32));
    jb_rj(17);
    jne_rj(8); // 2
    cmp_reg32_imm32(rs1, (uint32_t)imm); // 6
    jb_rj(7); // 2
    mov_reg32_imm32(rt, 0); // 5
    jmp_imm_short(5); // 2
    mov_reg32_imm32(rt, 1); // 5
#endif
}

void genandi()
{
#ifdef INTERPRET_ANDI
    gencallinterp((uint32_t)ANDI, 0);
#else
    int32_t rs = allocate_register((uint32_t*)dst->f.i.rs);
    int32_t rt = allocate_register_w((uint32_t*)dst->f.i.rt);

    mov_reg32_reg32(rt, rs);
    and_reg32_imm32(rt, (uint16_t)dst->f.i.immediate);
#endif
}

void genori()
{
#ifdef INTERPRET_ORI
    gencallinterp((uint32_t)ORI, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);
    int32_t rt1 = allocate_64_register1_w((uint32_t*)dst->f.i.rt);
    int32_t rt2 = allocate_64_register2_w((uint32_t*)dst->f.i.rt);

    mov_reg32_reg32(rt1, rs1);
    mov_reg32_reg32(rt2, rs2);
    or_reg32_imm32(rt1, (uint16_t)dst->f.i.immediate);
#endif
}

void genxori()
{
#ifdef INTERPRET_XORI
    gencallinterp((uint32_t)XORI, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);
    int32_t rt1 = allocate_64_register1_w((uint32_t*)dst->f.i.rt);
    int32_t rt2 = allocate_64_register2_w((uint32_t*)dst->f.i.rt);

    mov_reg32_reg32(rt1, rs1);
    mov_reg32_reg32(rt2, rs2);
    xor_reg32_imm32(rt1, (uint16_t)dst->f.i.immediate);
#endif
}

void genlui()
{
#ifdef INTERPRET_LUI
    gencallinterp((uint32_t)LUI, 0);
#else
    int32_t rt = allocate_register_w((uint32_t*)dst->f.i.rt);

    mov_reg32_imm32(rt, (uint32_t)dst->f.i.immediate << 16);
#endif
}

void gentestl()
{
    uint32_t temp, temp2;

    cmp_m32_imm32((uint32_t*)(&branch_taken), 0);
    je_near_rj(0);
    temp = code_length;
    gendelayslot();
    mov_m32_imm32((void*)(&last_addr), dst->addr + (dst - 1)->f.i.immediate * 4);
    gencheck_interrupt((uint32_t)(dst + (dst - 1)->f.i.immediate));
    jmp(dst->addr + (dst - 1)->f.i.immediate * 4);

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;
    genupdate_count(dst->addr - 4);
    mov_m32_imm32((void*)(&last_addr), dst->addr + 4);
    gencheck_interrupt((uint32_t)(dst + 1));
    jmp(dst->addr + 4);
}

void genbeql()
{
#ifdef INTERPRET_BEQL
    gencallinterp((uint32_t)BEQL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BEQL, 1);
        return;
    }

    genbeq_test();
    free_all_registers();
    gentestl();
#endif
}

void gentestl_out()
{
    uint32_t temp, temp2;

    cmp_m32_imm32((uint32_t*)(&branch_taken), 0);
    je_near_rj(0);
    temp = code_length;
    gendelayslot();
    mov_m32_imm32((void*)(&last_addr), dst->addr + (dst - 1)->f.i.immediate * 4);
    gencheck_interrupt_out(dst->addr + (dst - 1)->f.i.immediate * 4);
    mov_m32_imm32(&jump_to_address, dst->addr + (dst - 1)->f.i.immediate * 4);
    mov_m32_imm32((uint32_t*)(&PC), (uint32_t)(dst + 1));
    mov_reg32_imm32(EAX, (uint32_t)jump_to_func);
    call_reg32(EAX);

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;
    genupdate_count(dst->addr - 4);
    mov_m32_imm32((void*)(&last_addr), dst->addr + 4);
    gencheck_interrupt((uint32_t)(dst + 1));
    jmp(dst->addr + 4);
}

void genbeql_out()
{
#ifdef INTERPRET_BEQL_OUT
    gencallinterp((uint32_t)BEQL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BEQL_OUT, 1);
        return;
    }

    genbeq_test();
    free_all_registers();
    gentestl_out();
#endif
}

void genbeql_idle()
{
#ifdef INTERPRET_BEQL_IDLE
    gencallinterp((uint32_t)BEQL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BEQL_IDLE, 1);
        return;
    }

    genbeq_test();
    gentest_idle();
    genbeql();
#endif
}

void genbnel()
{
#ifdef INTERPRET_BNEL
    gencallinterp((uint32_t)BNEL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BNEL, 1);
        return;
    }

    genbne_test();
    free_all_registers();
    gentestl();
#endif
}

void genbnel_out()
{
#ifdef INTERPRET_BNEL_OUT
    gencallinterp((uint32_t)BNEL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BNEL_OUT, 1);
        return;
    }

    genbne_test();
    free_all_registers();
    gentestl_out();
#endif
}

void genbnel_idle()
{
#ifdef INTERPRET_BNEL_IDLE
    gencallinterp((uint32_t)BNEL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BNEL_IDLE, 1);
        return;
    }

    genbne_test();
    gentest_idle();
    genbnel();
#endif
}

void genblezl()
{
#ifdef INTERPRET_BLEZL
    gencallinterp((uint32_t)BLEZL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLEZL, 1);
        return;
    }

    genblez_test();
    free_all_registers();
    gentestl();
#endif
}

void genblezl_out()
{
#ifdef INTERPRET_BLEZL_OUT
    gencallinterp((uint32_t)BLEZL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLEZL_OUT, 1);
        return;
    }

    genblez_test();
    free_all_registers();
    gentestl_out();
#endif
}

void genblezl_idle()
{
#ifdef INTERPRET_BLEZL_IDLE
    gencallinterp((uint32_t)BLEZL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BLEZL_IDLE, 1);
        return;
    }

    genblez_test();
    gentest_idle();
    genblezl();
#endif
}

void genbgtzl()
{
#ifdef INTERPRET_BGTZL
    gencallinterp((uint32_t)BGTZL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGTZL, 1);
        return;
    }

    genbgtz_test();
    free_all_registers();
    gentestl();
#endif
}

void genbgtzl_out()
{
#ifdef INTERPRET_BGTZL_OUT
    gencallinterp((uint32_t)BGTZL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGTZL_OUT, 1);
        return;
    }

    genbgtz_test();
    free_all_registers();
    gentestl_out();
#endif
}

void genbgtzl_idle()
{
#ifdef INTERPRET_BGTZL_IDLE
    gencallinterp((uint32_t)BGTZL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)BGTZL_IDLE, 1);
        return;
    }

    genbgtz_test();
    gentest_idle();
    genbgtzl();
#endif
}

void gendaddi()
{
#ifdef INTERPRET_DADDI
    gencallinterp((uint32_t)DADDI, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);
    int32_t rt1 = allocate_64_register1_w((uint32_t*)dst->f.i.rt);
    int32_t rt2 = allocate_64_register2_w((uint32_t*)dst->f.i.rt);

    mov_reg32_reg32(rt1, rs1);
    mov_reg32_reg32(rt2, rs2);
    add_reg32_imm32(rt1, dst->f.i.immediate);
    adc_reg32_imm32(rt2, (int32_t)dst->f.i.immediate >> 31);
#endif
}

void gendaddiu()
{
#ifdef INTERPRET_DADDIU
    gencallinterp((uint32_t)DADDIU, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.i.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.i.rs);
    int32_t rt1 = allocate_64_register1_w((uint32_t*)dst->f.i.rt);
    int32_t rt2 = allocate_64_register2_w((uint32_t*)dst->f.i.rt);

    mov_reg32_reg32(rt1, rs1);
    mov_reg32_reg32(rt2, rs2);
    add_reg32_imm32(rt1, dst->f.i.immediate);
    adc_reg32_imm32(rt2, (int32_t)dst->f.i.immediate >> 31);
#endif
}

void genldl() { gencallinterp((uint32_t)LDL, 0); }

void genldr() { gencallinterp((uint32_t)LDR, 0); }

void genlb()
{
#ifdef INTERPRET_LB
    gencallinterp((uint32_t)LB, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t*)dst->f.i.rs);
    add_eax_imm32((int32_t)dst->f.i.immediate);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)readmemb);
        cmp_reg32_imm32(EAX, (uint32_t)read_rdramb);
    }
    je_rj(47);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_m32_imm32((uint32_t*)(&rdword), (uint32_t)dst->f.i.rt); // 10
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)readmemb); // 7
    call_reg32(EBX); // 2
    movsx_reg32_m8(EAX, (unsigned char*)dst->f.i.rt); // 7
    jmp_imm_short(16); // 2

    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    xor_reg8_imm8(BL, 3); // 3
    movsx_reg32_8preg32pimm32(EAX, EBX, (uint32_t)rdram); // 7

    set_register_state(EAX, (uint32_t*)dst->f.i.rt, 1);
#endif
}

void genlh()
{
#ifdef INTERPRET_LH
    gencallinterp((uint32_t)LH, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t*)dst->f.i.rs);
    add_eax_imm32((int32_t)dst->f.i.immediate);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)readmemh);
        cmp_reg32_imm32(EAX, (uint32_t)read_rdramh);
    }
    je_rj(47);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_m32_imm32((uint32_t*)(&rdword), (uint32_t)dst->f.i.rt); // 10
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)readmemh); // 7
    call_reg32(EBX); // 2
    movsx_reg32_m16(EAX, (uint16_t*)dst->f.i.rt); // 7
    jmp_imm_short(16); // 2

    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    xor_reg8_imm8(BL, 2); // 3
    movsx_reg32_16preg32pimm32(EAX, EBX, (uint32_t)rdram); // 7

    set_register_state(EAX, (uint32_t*)dst->f.i.rt, 1);
#endif
}

void genlwl() { gencallinterp((uint32_t)LWL, 0); }

void genlw()
{
#ifdef INTERPRET_LW
    gencallinterp((uint32_t)LW, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t*)dst->f.i.rs);
    add_eax_imm32((int32_t)dst->f.i.immediate);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)readmem);
        cmp_reg32_imm32(EAX, (uint32_t)read_rdram);
    }
    je_rj(45);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_m32_imm32((uint32_t*)(&rdword), (uint32_t)dst->f.i.rt); // 10
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)readmem); // 7
    call_reg32(EBX); // 2
    mov_eax_memoffs32((uint32_t*)(dst->f.i.rt)); // 5
    jmp_imm_short(12); // 2

    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    mov_reg32_preg32pimm32(EAX, EBX, (uint32_t)rdram); // 6

    set_register_state(EAX, (uint32_t*)dst->f.i.rt, 1);
#endif
}

void genlbu()
{
#ifdef INTERPRET_LBU
    gencallinterp((uint32_t)LBU, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t*)dst->f.i.rs);
    add_eax_imm32((int32_t)dst->f.i.immediate);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)readmemb);
        cmp_reg32_imm32(EAX, (uint32_t)read_rdramb);
    }
    je_rj(46);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_m32_imm32((uint32_t*)(&rdword), (uint32_t)dst->f.i.rt); // 10
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)readmemb); // 7
    call_reg32(EBX); // 2
    mov_reg32_m32(EAX, (uint32_t*)dst->f.i.rt); // 6
    jmp_imm_short(15); // 2

    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    xor_reg8_imm8(BL, 3); // 3
    mov_reg32_preg32pimm32(EAX, EBX, (uint32_t)rdram); // 6

    and_eax_imm32(0xFF);

    set_register_state(EAX, (uint32_t*)dst->f.i.rt, 1);
#endif
}

void genlhu()
{
#ifdef INTERPRET_LHU
    gencallinterp((uint32_t)LHU, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t*)dst->f.i.rs);
    add_eax_imm32((int32_t)dst->f.i.immediate);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)readmemh);
        cmp_reg32_imm32(EAX, (uint32_t)read_rdramh);
    }
    je_rj(46);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_m32_imm32((uint32_t*)(&rdword), (uint32_t)dst->f.i.rt); // 10
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)readmemh); // 7
    call_reg32(EBX); // 2
    mov_reg32_m32(EAX, (uint32_t*)dst->f.i.rt); // 6
    jmp_imm_short(15); // 2

    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    xor_reg8_imm8(BL, 2); // 3
    mov_reg32_preg32pimm32(EAX, EBX, (uint32_t)rdram); // 6

    and_eax_imm32(0xFFFF);

    set_register_state(EAX, (uint32_t*)dst->f.i.rt, 1);
#endif
}

void genlwr() { gencallinterp((uint32_t)LWR, 0); }

void genlwu()
{
#ifdef INTERPRET_LWU
    gencallinterp((uint32_t)LWU, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t*)dst->f.i.rs);
    add_eax_imm32((int32_t)dst->f.i.immediate);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)readmem);
        cmp_reg32_imm32(EAX, (uint32_t)read_rdram);
    }
    je_rj(45);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_m32_imm32((uint32_t*)(&rdword), (uint32_t)dst->f.i.rt); // 10
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)readmem); // 7
    call_reg32(EBX); // 2
    mov_eax_memoffs32((uint32_t*)(dst->f.i.rt)); // 5
    jmp_imm_short(12); // 2

    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    mov_reg32_preg32pimm32(EAX, EBX, (uint32_t)rdram); // 6

    xor_reg32_reg32(EBX, EBX);

    set_64_register_state(EAX, EBX, (uint32_t*)dst->f.i.rt, 1);
#endif
}

void gensb()
{
#ifdef INTERPRET_SB
    gencallinterp((uint32_t)SB, 0);
#else
    free_all_registers();
    simplify_access();
    mov_reg8_m8(CL, (unsigned char*)dst->f.i.rt);
    mov_eax_memoffs32((uint32_t*)dst->f.i.rs);
    add_eax_imm32((int32_t)dst->f.i.immediate);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)writememb);
        cmp_reg32_imm32(EAX, (uint32_t)write_rdramb);
    }
    je_rj(41);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_m8_reg8((unsigned char*)(&g_byte), CL); // 6
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)writememb); // 7
    call_reg32(EBX); // 2
    mov_eax_memoffs32((uint32_t*)(&address)); // 5
    jmp_imm_short(17); // 2

    mov_reg32_reg32(EAX, EBX); // 2
    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    xor_reg8_imm8(BL, 3); // 3
    mov_preg32pimm32_reg8(EBX, (uint32_t)rdram, CL); // 6

    mov_reg32_reg32(EBX, EAX);
    shr_reg32_imm8(EBX, 12);
    cmp_preg32pimm32_imm8(EBX, (uint32_t)invalid_code, 0);
    jne_rj(54);
    mov_reg32_reg32(ECX, EBX); // 2
    shl_reg32_imm8(EBX, 2); // 3
    mov_reg32_preg32pimm32(EBX, EBX, (uint32_t)blocks); // 6
    mov_reg32_preg32pimm32(EBX, EBX,
                           (int32_t)&actual->block - (int32_t)actual); // 6
    and_eax_imm32(0xFFF); // 5
    shr_reg32_imm8(EAX, 2); // 3
    mov_reg32_imm32(EDX, sizeof(precomp_instr)); // 5
    mul_reg32(EDX); // 2
    mov_reg32_preg32preg32pimm32(EAX, EAX, EBX,
                                 (int32_t)&dst->ops - (int32_t)dst); // 7
    cmp_reg32_imm32(EAX, (uint32_t)NOTCOMPILED); // 6
    je_rj(7); // 2
    mov_preg32pimm32_imm8(ECX, (uint32_t)invalid_code, 1); // 7
#endif
}

void gensh()
{
#ifdef INTERPRET_SH
    gencallinterp((uint32_t)SH, 0);
#else
    free_all_registers();
    simplify_access();
    mov_reg16_m16(CX, (uint16_t*)dst->f.i.rt);
    mov_eax_memoffs32((uint32_t*)dst->f.i.rs);
    add_eax_imm32((int32_t)dst->f.i.immediate);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)writememh);
        cmp_reg32_imm32(EAX, (uint32_t)write_rdramh);
    }
    je_rj(42);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_m16_reg16((uint16_t*)(&hword), CX); // 7
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)writememh); // 7
    call_reg32(EBX); // 2
    mov_eax_memoffs32((uint32_t*)(&address)); // 5
    jmp_imm_short(18); // 2

    mov_reg32_reg32(EAX, EBX); // 2
    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    xor_reg8_imm8(BL, 2); // 3
    mov_preg32pimm32_reg16(EBX, (uint32_t)rdram, CX); // 7

    mov_reg32_reg32(EBX, EAX);
    shr_reg32_imm8(EBX, 12);
    cmp_preg32pimm32_imm8(EBX, (uint32_t)invalid_code, 0);
    jne_rj(54);
    mov_reg32_reg32(ECX, EBX); // 2
    shl_reg32_imm8(EBX, 2); // 3
    mov_reg32_preg32pimm32(EBX, EBX, (uint32_t)blocks); // 6
    mov_reg32_preg32pimm32(EBX, EBX,
                           (int32_t)&actual->block - (int32_t)actual); // 6
    and_eax_imm32(0xFFF); // 5
    shr_reg32_imm8(EAX, 2); // 3
    mov_reg32_imm32(EDX, sizeof(precomp_instr)); // 5
    mul_reg32(EDX); // 2
    mov_reg32_preg32preg32pimm32(EAX, EAX, EBX,
                                 (int32_t)&dst->ops - (int32_t)dst); // 7
    cmp_reg32_imm32(EAX, (uint32_t)NOTCOMPILED); // 6
    je_rj(7); // 2
    mov_preg32pimm32_imm8(ECX, (uint32_t)invalid_code, 1); // 7
#endif
}

void genswl() { gencallinterp((uint32_t)SWL, 0); }

void gensw()
{
#ifdef INTERPRET_SW
    gencallinterp((uint32_t)SW, 0);
#else
    free_all_registers();
    simplify_access();
    mov_reg32_m32(ECX, (uint32_t*)dst->f.i.rt);
    mov_eax_memoffs32((uint32_t*)dst->f.i.rs);
    add_eax_imm32((int32_t)dst->f.i.immediate);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)writemem);
        cmp_reg32_imm32(EAX, (uint32_t)write_rdram);
    }
    je_rj(41);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_m32_reg32((uint32_t*)(&word), ECX); // 6
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)writemem); // 7
    call_reg32(EBX); // 2
    mov_eax_memoffs32((uint32_t*)(&address)); // 5
    jmp_imm_short(14); // 2

    mov_reg32_reg32(EAX, EBX); // 2
    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    mov_preg32pimm32_reg32(EBX, (uint32_t)rdram, ECX); // 6

    mov_reg32_reg32(EBX, EAX);
    shr_reg32_imm8(EBX, 12);
    cmp_preg32pimm32_imm8(EBX, (uint32_t)invalid_code, 0);
    jne_rj(54);
    mov_reg32_reg32(ECX, EBX); // 2
    shl_reg32_imm8(EBX, 2); // 3
    mov_reg32_preg32pimm32(EBX, EBX, (uint32_t)blocks); // 6
    mov_reg32_preg32pimm32(EBX, EBX,
                           (int32_t)&actual->block - (int32_t)actual); // 6
    and_eax_imm32(0xFFF); // 5
    shr_reg32_imm8(EAX, 2); // 3
    mov_reg32_imm32(EDX, sizeof(precomp_instr)); // 5
    mul_reg32(EDX); // 2
    mov_reg32_preg32preg32pimm32(EAX, EAX, EBX,
                                 (int32_t)&dst->ops - (int32_t)dst); // 7
    cmp_reg32_imm32(EAX, (uint32_t)NOTCOMPILED); // 6
    je_rj(7); // 2
    mov_preg32pimm32_imm8(ECX, (uint32_t)invalid_code, 1); // 7
#endif
}

void gensdl() { gencallinterp((uint32_t)SDL, 0); }

void gensdr() { gencallinterp((uint32_t)SDR, 0); }

void genswr() { gencallinterp((uint32_t)SWR, 0); }

#include <malloc.h>

inline void put8gr(unsigned char octet)
{
    (*inst_pointer)[code_length] = octet;
    code_length++;
    if (code_length == max_code_length)
    {
        max_code_length += 1000;
        *inst_pointer = (unsigned char*)realloc(*inst_pointer, max_code_length);
    }
}

void gencheck_cop1_unusable()
{
    uint32_t temp, temp2;
    free_all_registers();
    simplify_access();
    test_m32_imm32((uint32_t*)&core_Status, 0x20000000);
    jne_rj(0);
    temp = code_length;

    gencallinterp((uint32_t)check_cop1_unusable, 0);

    temp2 = code_length;
    code_length = temp - 1;
    put8gr(temp2 - temp);
    code_length = temp2;
}

void genlwc1()
{
#ifdef INTERPRET_LWC1
    gencallinterp((uint32_t)LWC1, 0);
#else
    gencheck_cop1_unusable();

    mov_eax_memoffs32((uint32_t*)(&reg[dst->f.lf.base]));
    add_eax_imm32((int32_t)dst->f.lf.offset);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)readmem);
        cmp_reg32_imm32(EAX, (uint32_t)read_rdram);
    }
    je_rj(42);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_reg32_m32(EDX, (uint32_t*)(&reg_cop1_simple[dst->f.lf.ft])); // 6
    mov_m32_reg32((uint32_t*)(&rdword), EDX); // 6
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)readmem); // 7
    call_reg32(EBX); // 2
    jmp_imm_short(20); // 2

    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    mov_reg32_preg32pimm32(EAX, EBX, (uint32_t)rdram); // 6
    mov_reg32_m32(EBX, (uint32_t*)(&reg_cop1_simple[dst->f.lf.ft])); // 6
    mov_preg32_reg32(EBX, EAX); // 2
#endif
}

void genldc1()
{
#ifdef INTERPRET_LDC1
    gencallinterp((uint32_t)LDC1, 0);
#else
    gencheck_cop1_unusable();

    mov_eax_memoffs32((uint32_t*)(&reg[dst->f.lf.base]));
    add_eax_imm32((int32_t)dst->f.lf.offset);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)readmemd);
        cmp_reg32_imm32(EAX, (uint32_t)read_rdramd);
    }
    je_rj(42);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_reg32_m32(EDX, (uint32_t*)(&reg_cop1_double[dst->f.lf.ft])); // 6
    mov_m32_reg32((uint32_t*)(&rdword), EDX); // 6
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)readmemd); // 7
    call_reg32(EBX); // 2
    jmp_imm_short(32); // 2

    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    mov_reg32_preg32pimm32(EAX, EBX, ((uint32_t)rdram) + 4); // 6
    mov_reg32_preg32pimm32(ECX, EBX, ((uint32_t)rdram)); // 6
    mov_reg32_m32(EBX, (uint32_t*)(&reg_cop1_double[dst->f.lf.ft])); // 6
    mov_preg32_reg32(EBX, EAX); // 2
    mov_preg32pimm32_reg32(EBX, 4, ECX); // 6
#endif
}

void gencache() {}

void genld()
{
#ifdef INTERPRET_LD
    gencallinterp((uint32_t)LD, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t*)dst->f.i.rs);
    add_eax_imm32((int32_t)dst->f.i.immediate);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)readmemd);
        cmp_reg32_imm32(EAX, (uint32_t)read_rdramd);
    }
    je_rj(51);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_m32_imm32((uint32_t*)(&rdword), (uint32_t)dst->f.i.rt); // 10
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)readmemd); // 7
    call_reg32(EBX); // 2
    mov_eax_memoffs32((uint32_t*)(dst->f.i.rt)); // 5
    mov_reg32_m32(ECX, (uint32_t*)(dst->f.i.rt) + 1); // 6
    jmp_imm_short(18); // 2

    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    mov_reg32_preg32pimm32(EAX, EBX, ((uint32_t)rdram) + 4); // 6
    mov_reg32_preg32pimm32(ECX, EBX, ((uint32_t)rdram)); // 6

    set_64_register_state(EAX, ECX, (uint32_t*)dst->f.i.rt, 1);
#endif
}

void genswc1()
{
#ifdef INTERPRET_SWC1
    gencallinterp((uint32_t)SWC1, 0);
#else
    gencheck_cop1_unusable();

    mov_reg32_m32(EDX, (uint32_t*)(&reg_cop1_simple[dst->f.lf.ft]));
    mov_reg32_preg32(ECX, EDX);
    mov_eax_memoffs32((uint32_t*)(&reg[dst->f.lf.base]));
    add_eax_imm32((int32_t)dst->f.lf.offset);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)writemem);
        cmp_reg32_imm32(EAX, (uint32_t)write_rdram);
    }
    je_rj(41);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_m32_reg32((uint32_t*)(&word), ECX); // 6
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)writemem); // 7
    call_reg32(EBX); // 2
    mov_eax_memoffs32((uint32_t*)(&address)); // 5
    jmp_imm_short(14); // 2

    mov_reg32_reg32(EAX, EBX); // 2
    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    mov_preg32pimm32_reg32(EBX, (uint32_t)rdram, ECX); // 6

    mov_reg32_reg32(EBX, EAX);
    shr_reg32_imm8(EBX, 12);
    cmp_preg32pimm32_imm8(EBX, (uint32_t)invalid_code, 0);
    jne_rj(54);
    mov_reg32_reg32(ECX, EBX); // 2
    shl_reg32_imm8(EBX, 2); // 3
    mov_reg32_preg32pimm32(EBX, EBX, (uint32_t)blocks); // 6
    mov_reg32_preg32pimm32(EBX, EBX,
                           (int32_t)&actual->block - (int32_t)actual); // 6
    and_eax_imm32(0xFFF); // 5
    shr_reg32_imm8(EAX, 2); // 3
    mov_reg32_imm32(EDX, sizeof(precomp_instr)); // 5
    mul_reg32(EDX); // 2
    mov_reg32_preg32preg32pimm32(EAX, EAX, EBX,
                                 (int32_t)&dst->ops - (int32_t)dst); // 7
    cmp_reg32_imm32(EAX, (uint32_t)NOTCOMPILED); // 6
    je_rj(7); // 2
    mov_preg32pimm32_imm8(ECX, (uint32_t)invalid_code, 1); // 7
#endif
}

void gensdc1()
{
#ifdef INTERPRET_SDC1
    gencallinterp((uint32_t)SDC1, 0);
#else
    gencheck_cop1_unusable();

    mov_reg32_m32(ESI, (uint32_t*)(&reg_cop1_double[dst->f.lf.ft]));
    mov_reg32_preg32(ECX, ESI);
    mov_reg32_preg32pimm32(EDX, ESI, 4);
    mov_eax_memoffs32((uint32_t*)(&reg[dst->f.lf.base]));
    add_eax_imm32((int32_t)dst->f.lf.offset);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)writememd);
        cmp_reg32_imm32(EAX, (uint32_t)write_rdramd);
    }
    je_rj(47);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_m32_reg32((uint32_t*)(&dword), ECX); // 6
    mov_m32_reg32((uint32_t*)(&dword) + 1, EDX); // 6
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)writememd); // 7
    call_reg32(EBX); // 2
    mov_eax_memoffs32((uint32_t*)(&address)); // 5
    jmp_imm_short(20); // 2

    mov_reg32_reg32(EAX, EBX); // 2
    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    mov_preg32pimm32_reg32(EBX, ((uint32_t)rdram) + 4, ECX); // 6
    mov_preg32pimm32_reg32(EBX, ((uint32_t)rdram) + 0, EDX); // 6

    mov_reg32_reg32(EBX, EAX);
    shr_reg32_imm8(EBX, 12);
    cmp_preg32pimm32_imm8(EBX, (uint32_t)invalid_code, 0);
    jne_rj(54);
    mov_reg32_reg32(ECX, EBX); // 2
    shl_reg32_imm8(EBX, 2); // 3
    mov_reg32_preg32pimm32(EBX, EBX, (uint32_t)blocks); // 6
    mov_reg32_preg32pimm32(EBX, EBX,
                           (int32_t)&actual->block - (int32_t)actual); // 6
    and_eax_imm32(0xFFF); // 5
    shr_reg32_imm8(EAX, 2); // 3
    mov_reg32_imm32(EDX, sizeof(precomp_instr)); // 5
    mul_reg32(EDX); // 2
    mov_reg32_preg32preg32pimm32(EAX, EAX, EBX,
                                 (int32_t)&dst->ops - (int32_t)dst); // 7
    cmp_reg32_imm32(EAX, (uint32_t)NOTCOMPILED); // 6
    je_rj(7); // 2
    mov_preg32pimm32_imm8(ECX, (uint32_t)invalid_code, 1); // 7
#endif
}

void gensd()
{
#ifdef INTERPRET_SD
    gencallinterp((uint32_t)SD, 0);
#else
    free_all_registers();
    simplify_access();

    mov_reg32_m32(ECX, (uint32_t*)dst->f.i.rt);
    mov_reg32_m32(EDX, ((uint32_t*)dst->f.i.rt) + 1);
    mov_eax_memoffs32((uint32_t*)dst->f.i.rs);
    add_eax_imm32((int32_t)dst->f.i.immediate);
    mov_reg32_reg32(EBX, EAX);
    if (fast_memory)
    {
        and_eax_imm32(0xDF800000);
        cmp_eax_imm32(0x80000000);
    }
    else
    {
        shr_reg32_imm8(EAX, 16);
        mov_reg32_preg32x4pimm32(EAX, EAX, (uint32_t)writememd);
        cmp_reg32_imm32(EAX, (uint32_t)write_rdramd);
    }
    je_rj(47);

    mov_m32_imm32((void*)(&PC), (uint32_t)(dst + 1)); // 10
    mov_m32_reg32((uint32_t*)(&address), EBX); // 6
    mov_m32_reg32((uint32_t*)(&dword), ECX); // 6
    mov_m32_reg32((uint32_t*)(&dword) + 1, EDX); // 6
    shr_reg32_imm8(EBX, 16); // 3
    mov_reg32_preg32x4pimm32(EBX, EBX, (uint32_t)writememd); // 7
    call_reg32(EBX); // 2
    mov_eax_memoffs32((uint32_t*)(&address)); // 5
    jmp_imm_short(20); // 2

    mov_reg32_reg32(EAX, EBX); // 2
    and_reg32_imm32(EBX, 0x7FFFFF); // 6
    mov_preg32pimm32_reg32(EBX, ((uint32_t)rdram) + 4, ECX); // 6
    mov_preg32pimm32_reg32(EBX, ((uint32_t)rdram) + 0, EDX); // 6

    mov_reg32_reg32(EBX, EAX);
    shr_reg32_imm8(EBX, 12);
    cmp_preg32pimm32_imm8(EBX, (uint32_t)invalid_code, 0);
    jne_rj(54);
    mov_reg32_reg32(ECX, EBX); // 2
    shl_reg32_imm8(EBX, 2); // 3
    mov_reg32_preg32pimm32(EBX, EBX, (uint32_t)blocks); // 6
    mov_reg32_preg32pimm32(EBX, EBX,
                           (int32_t)&actual->block - (int32_t)actual); // 6
    and_eax_imm32(0xFFF); // 5
    shr_reg32_imm8(EAX, 2); // 3
    mov_reg32_imm32(EDX, sizeof(precomp_instr)); // 5
    mul_reg32(EDX); // 2
    mov_reg32_preg32preg32pimm32(EAX, EAX, EBX,
                                 (int32_t)&dst->ops - (int32_t)dst); // 7
    cmp_reg32_imm32(EAX, (uint32_t)NOTCOMPILED); // 6
    je_rj(7); // 2
    mov_preg32pimm32_imm8(ECX, (uint32_t)invalid_code, 1); // 7
#endif
}

void genll() { gencallinterp((uint32_t)LL, 0); }

void gensc() { gencallinterp((uint32_t)SC, 0); }
