/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <Core.hpp>
#include <Memory/Memory.hpp>
#include <R4300/Interrupt.hpp>
#include <R4300/Macros.hpp>
#include <R4300/Ops.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/x86_64/Assemble.hpp>
#include <R4300/x86_64/RegCache.hpp>
#include <Alloc.hpp>


extern uint32_t src; // recomp.c

precomp_instr fake_instr;
static int32_t eax, ebx, ecx, edx, esp, ebp, esi, edi;

int32_t branch_taken;

// Emit the store self-modifying-code invalidation check as a call to the C runtime
// helper dyna_mem_invalidate (which reads the `address` global). Assumes EAX holds
// the store's target address. The store already flushed the register cache
// (simplify_access), so clobbering caller-saved registers here is safe. Replaces the
// old inline sequence that read 64-bit pointers (blocks[page]->block[i].ops) as
// 32-bit and indexed blocks[] with a x4 scale -> truncated pointers -> crash.
static void gen_check_invalidate()
{
    mov_m32_reg32((void *)&address, EAX);
    mov_reg64_imm64(EAX, (uintptr_t)dyna_mem_invalidate);
    call_reg64(EAX);
}

void gennotcompiled()
{
    free_all_registers();
    simplify_access();

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst));
    mov_reg64_imm64(EAX, (uintptr_t)NOTCOMPILED);
    call_reg64(EAX);
}

void genlink_subblock()
{
    free_all_registers();
    jmp(dst->addr + 4);
}

void gendebug()
{
    // free_all_registers();
    mov_m64_reg64((void *)&eax, EAX);
    mov_m64_reg64((void *)&ebx, EBX);
    mov_m64_reg64((void *)&ecx, ECX);
    mov_m64_reg64((void *)&edx, EDX);
    mov_m64_reg64((void *)&esp, ESP);
    mov_m64_reg64((void *)&ebp, EBP);
    mov_m64_reg64((void *)&esi, ESI);
    mov_m64_reg64((void *)&edi, EDI);

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst));
    mov_m32_imm32((void *)(&vr_op), (uintptr_t)(src));
    mov_reg64_imm64(EAX, (uintptr_t)debug);
    call_reg64(EAX);

    mov_reg64_m64(EAX, (void *)&eax);
    mov_reg64_m64(EBX, (void *)&ebx);
    mov_reg64_m64(ECX, (void *)&ecx);
    mov_reg64_m64(EDX, (void *)&edx);
    mov_reg64_m64(ESP, (void *)&esp);
    mov_reg64_m64(EBP, (void *)&ebp);
    mov_reg64_m64(ESI, (void *)&esi);
    mov_reg64_m64(EDI, (void *)&edi);
}

void gencallinterp(uintptr_t addr, int32_t jump)
{
    free_all_registers();
    simplify_access();
    if (jump) mov_m32_imm32((void *)(&dyna_interp), 1);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst));
    mov_reg64_imm64(EAX, addr);
    call_reg64(EAX);
    if (jump)
    {
        mov_m32_imm32((void *)(&dyna_interp), 0);
        mov_reg64_imm64(EAX, (uintptr_t)dyna_jump);
        call_reg64(EAX);
    }
}

void genupdate_count(uint32_t addr)
{
    mov_reg32_imm32(EAX, addr);
    sub_reg32_m32(EAX, (void *)(&last_addr));
    shr_reg32_imm8(EAX, 1);
    add_m32_reg32((void *)(&core_Count), EAX);
}

void gendelayslot()
{
    mov_m32_imm32((void *)(&delay_slot), 1);
    recompile_opcode();

    free_all_registers();
    genupdate_count(dst->addr + 4);

    mov_m32_imm32((void *)(&delay_slot), 0);
}

void genni()
{
}

void genreserved()
{
}

void genfin_block()
{
    gencallinterp((uintptr_t)FIN_BLOCK, 0);
}

void gencheck_interrupt(uintptr_t instr_structure)
{
    mov_eax_memoffs32((void *)(&next_interrupt));
    cmp_reg32_m32(EAX, (void *)&core_Count);
    // Adjusted displacement: 23 (mov_m64) + 10 (mov_reg64) + 24 (call_reg64) = 57 bytes
    ja_rj(57);
    mov_m64_imm64((void *)(&PC), instr_structure); // 23
    mov_reg64_imm64(EAX, (uintptr_t)gen_interrupt);     // 10
    call_reg64(EAX);                                   // 24
}

void gencheck_interrupt_out(uint32_t addr)
{
    mov_eax_memoffs32((void *)(&next_interrupt));
    cmp_reg32_m32(EAX, (void *)&core_Count);
    // Adjusted displacement: 17 (mov_m32) + 23 (mov_m64) + 10 (mov_reg64) + 24 (call_reg64) = 74 bytes
    ja_rj(74);
    mov_m32_imm32((void *)(&fake_instr.addr), addr);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(&fake_instr));
    mov_reg64_imm64(EAX, (uintptr_t)gen_interrupt);
    call_reg64(EAX);
}

void gencheck_interrupt_reg() // addr is in EAX
{
    mov_reg32_m32(EBX, (void *)&next_interrupt);
    cmp_reg32_m32(EBX, (void *)&core_Count);
    // Adjusted displacement: 23 (mov_m64) + 13 (mov_memoffs32) + 10 (mov_reg64) + 24 (call_reg64) = 70 bytes
    ja_rj(70);
    mov_memoffs32_eax((void *)(&fake_instr.addr));         // 13
    mov_m64_imm64((void *)(&PC), (uintptr_t)(&fake_instr)); // 23
    mov_reg64_imm64(EAX, (uintptr_t)gen_interrupt);             // 10
    call_reg64(EAX);                                           // 24
}

void gennop()
{
}

void genj()
{
#ifdef INTERPRET_J
    gencallinterp((uintptr_t)J, 1);
#else
    uint32_t naddr;

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)J, 1);
        return;
    }

    gendelayslot();
    naddr = ((dst - 1)->f.j.inst_index << 2) | (dst->addr & 0xF0000000);

    mov_m32_imm32((void *)(&last_addr), naddr);
    gencheck_interrupt((uintptr_t)&actual->block[(naddr - actual->start) / 4]);
    jmp(naddr);
#endif
}

void genj_out()
{
#ifdef INTERPRET_J_OUT
    gencallinterp((uintptr_t)J_OUT, 1);
#else
    uint32_t naddr;

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)J_OUT, 1);
        return;
    }

    gendelayslot();
    naddr = ((dst - 1)->f.j.inst_index << 2) | (dst->addr & 0xF0000000);

    mov_m32_imm32((void *)(&last_addr), naddr);
    gencheck_interrupt_out(naddr);
    mov_m32_imm32(&jump_to_address, naddr);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_reg64_imm64(EAX, (uintptr_t)jump_to_func);
    call_reg64(EAX);
#endif
}

void genj_idle()
{
#ifdef INTERPRET_J_IDLE
    gencallinterp((uintptr_t)J_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)J_IDLE, 1);
        return;
    }

    mov_eax_memoffs32((void *)(&next_interrupt));
    sub_reg32_m32(EAX, (void *)(&core_Count));
    cmp_reg32_imm8(EAX, 3);
    jbe_rj(0);
    int32_t j = code_length - 1;

    and_eax_imm32(0xFFFFFFFC);
    add_m32_reg32((void *)(&core_Count), EAX);
    rj_patch(j);

    genj();
#endif
}

void genjal()
{
#ifdef INTERPRET_JAL
    gencallinterp((uintptr_t)JAL, 1);
#else
    uint32_t naddr;

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)JAL, 1);
        return;
    }

    gendelayslot();

    mov_m32_imm32((uint32_t *)(reg + 31), dst->addr + 4);
    if (((dst->addr + 4) & 0x80000000))
        mov_m32_imm32((uint32_t *)(&reg[31]) + 1, 0xFFFFFFFF);
    else
        mov_m32_imm32((uint32_t *)(&reg[31]) + 1, 0);

    naddr = ((dst - 1)->f.j.inst_index << 2) | (dst->addr & 0xF0000000);

    mov_m32_imm32((void *)(&last_addr), naddr);
    gencheck_interrupt((uintptr_t)&actual->block[(naddr - actual->start) / 4]);
    jmp(naddr);
#endif
}

void genjal_out()
{
#ifdef INTERPRET_JAL_OUT
    gencallinterp((uintptr_t)JAL_OUT, 1);
#else
    uint32_t naddr;

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)JAL_OUT, 1);
        return;
    }

    gendelayslot();

    mov_m32_imm32((uint32_t *)(reg + 31), dst->addr + 4);
    if (((dst->addr + 4) & 0x80000000))
        mov_m32_imm32((uint32_t *)(&reg[31]) + 1, 0xFFFFFFFF);
    else
        mov_m32_imm32((uint32_t *)(&reg[31]) + 1, 0);

    naddr = ((dst - 1)->f.j.inst_index << 2) | (dst->addr & 0xF0000000);

    mov_m32_imm32((void *)(&last_addr), naddr);
    gencheck_interrupt_out(naddr);
    mov_m32_imm32(&jump_to_address, naddr);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_reg64_imm64(EAX, (uintptr_t)jump_to_func);
    call_reg64(EAX);
#endif
}

void genjal_idle()
{
#ifdef INTERPRET_JAL_IDLE
    gencallinterp((uintptr_t)JAL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)JAL_IDLE, 1);
        return;
    }

    mov_eax_memoffs32((void *)(&next_interrupt));
    sub_reg32_m32(EAX, (void *)(&core_Count));
    cmp_reg32_imm8(EAX, 3);
    jbe_rj(0);
    int32_t j = code_length - 1;

    and_eax_imm32(0xFFFFFFFC);
    add_m32_reg32((void *)(&core_Count), EAX);
    rj_patch(j);

    genjal();
#endif
}

void genbeq_test()
{
    int32_t rs_64bit = is64((uintptr_t)dst->f.i.rs);
    int32_t rt_64bit = is64((uintptr_t)dst->f.i.rt);

    if (!rs_64bit && !rt_64bit)
    {
        int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
        int32_t rt = allocate_register((uintptr_t)dst->f.i.rt);

        cmp_reg32_reg32(rs, rt);
        jne_rj(0);
        int32_t j1 = code_length - 1;
        mov_m32_imm32((void *)(&branch_taken), 1);
        jmp_imm_short(0);
        int32_t je = code_length - 1;
        rj_patch(j1);
        mov_m32_imm32((void *)(&branch_taken), 0);
        rj_patch(je);
    }
    else if (rs_64bit == -1)
    {
        int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.i.rt);
        int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.i.rt);

        cmp_reg32_m32(rt1, (uint32_t *)dst->f.i.rs);
        jne_rj(0);
        int32_t j1 = code_length - 1;
        cmp_reg32_m32(rt2, ((uint32_t *)dst->f.i.rs) + 1);
        jne_rj(0);
        int32_t j2 = code_length - 1;
        mov_m32_imm32((void *)(&branch_taken), 1);
        jmp_imm_short(0);
        int32_t je = code_length - 1;
        rj_patch(j1);
        rj_patch(j2);
        mov_m32_imm32((void *)(&branch_taken), 0);
        rj_patch(je);
    }
    else if (rt_64bit == -1)
    {
        int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
        int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);

        cmp_reg32_m32(rs1, (uint32_t *)dst->f.i.rt);
        jne_rj(0);
        int32_t j1 = code_length - 1;
        cmp_reg32_m32(rs2, ((uint32_t *)dst->f.i.rt) + 1);
        jne_rj(0);
        int32_t j2 = code_length - 1;
        mov_m32_imm32((void *)(&branch_taken), 1);
        jmp_imm_short(0);
        int32_t je = code_length - 1;
        rj_patch(j1);
        rj_patch(j2);
        mov_m32_imm32((void *)(&branch_taken), 0);
        rj_patch(je);
    }
    else
    {
        int32_t rs1, rs2, rt1, rt2;
        if (!rs_64bit)
        {
            rt1 = allocate_64_register1((uintptr_t)dst->f.i.rt);
            rt2 = allocate_64_register2((uintptr_t)dst->f.i.rt);
            rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
            rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);
        }
        else
        {
            rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
            rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);
            rt1 = allocate_64_register1((uintptr_t)dst->f.i.rt);
            rt2 = allocate_64_register2((uintptr_t)dst->f.i.rt);
        }
        cmp_reg32_reg32(rs1, rt1);
        jne_rj(0);
        int32_t j1 = code_length - 1;
        cmp_reg32_reg32(rs2, rt2);
        jne_rj(0);
        int32_t j2 = code_length - 1;
        mov_m32_imm32((void *)(&branch_taken), 1);
        jmp_imm_short(0);
        int32_t je = code_length - 1;
        rj_patch(j1);
        rj_patch(j2);
        mov_m32_imm32((void *)(&branch_taken), 0);
        rj_patch(je);
    }
}

void gentest()
{
    uint32_t temp, temp2;

    cmp_m32_imm32((void *)(&branch_taken), 0);
    je_near_rj(0);
    temp = code_length;
    mov_m32_imm32((void *)(&last_addr), dst->addr + (dst - 1)->f.i.immediate * 4);
    gencheck_interrupt((uintptr_t)(dst + (dst - 1)->f.i.immediate));
    jmp(dst->addr + (dst - 1)->f.i.immediate * 4);

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;
    mov_m32_imm32((void *)(&last_addr), dst->addr + 4);
    gencheck_interrupt((uintptr_t)(dst + 1));
    jmp(dst->addr + 4);
}

void genbeq()
{
#ifdef INTERPRET_BEQ
    gencallinterp((uintptr_t)BEQ, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BEQ, 1);
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

    cmp_m32_imm32((void *)(&branch_taken), 0);
    je_near_rj(0);
    temp = code_length;
    mov_m32_imm32((void *)(&last_addr), dst->addr + (dst - 1)->f.i.immediate * 4);
    gencheck_interrupt_out(dst->addr + (dst - 1)->f.i.immediate * 4);
    mov_m32_imm32(&jump_to_address, dst->addr + (dst - 1)->f.i.immediate * 4);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_reg64_imm64(EAX, (uintptr_t)jump_to_func);
    call_reg64(EAX);

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;
    mov_m32_imm32((void *)(&last_addr), dst->addr + 4);
    gencheck_interrupt((uintptr_t)(dst + 1));
    jmp(dst->addr + 4);
}

void genbeq_out()
{
#ifdef INTERPRET_BEQ_OUT
    gencallinterp((uintptr_t)BEQ_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BEQ_OUT, 1);
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

    cmp_m32_imm32((void *)(&branch_taken), 0);
    je_near_rj(0);
    temp = code_length;

    mov_reg32_m32(reg, (void *)(&next_interrupt));
    sub_reg32_m32(reg, (void *)(&core_Count));
    cmp_reg32_imm8(reg, 3);
    jbe_rj(0);
    int32_t j = code_length - 1;

    and_reg32_imm32(reg, 0xFFFFFFFC);
    add_m32_reg32((void *)(&core_Count), reg);
    rj_patch(j);

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;
}

void genbeq_idle()
{
#ifdef INTERPRET_BEQ_IDLE
    gencallinterp((uintptr_t)BEQ_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BEQ_IDLE, 1);
        return;
    }

    genbeq_test();
    gentest_idle();
    genbeq();
#endif
}

void genbne_test()
{
    int32_t rs_64bit = is64((uintptr_t)dst->f.i.rs);
    int32_t rt_64bit = is64((uintptr_t)dst->f.i.rt);

    if (!rs_64bit && !rt_64bit)
    {
        int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
        int32_t rt = allocate_register((uintptr_t)dst->f.i.rt);

        cmp_reg32_reg32(rs, rt);
        je_rj(0);
        int32_t j1 = code_length - 1;
        mov_m32_imm32((void *)(&branch_taken), 1);
        jmp_imm_short(0);
        int32_t je = code_length - 1;
        rj_patch(j1);
        mov_m32_imm32((void *)(&branch_taken), 0);
        rj_patch(je);
    }
    else if (rs_64bit == -1)
    {
        int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.i.rt);
        int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.i.rt);

        cmp_reg32_m32(rt1, (uint32_t *)dst->f.i.rs);
        jne_rj(0);
        int32_t j1 = code_length - 1;
        cmp_reg32_m32(rt2, ((uint32_t *)dst->f.i.rs) + 1);
        jne_rj(0);
        int32_t j2 = code_length - 1;
        mov_m32_imm32((void *)(&branch_taken), 0);
        jmp_imm_short(0);
        int32_t je = code_length - 1;
        rj_patch(j1);
        rj_patch(j2);
        mov_m32_imm32((void *)(&branch_taken), 1);
        rj_patch(je);
    }
    else if (rt_64bit == -1)
    {
        int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
        int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);

        cmp_reg32_m32(rs1, (uint32_t *)dst->f.i.rt);
        jne_rj(0);
        int32_t j1 = code_length - 1;
        cmp_reg32_m32(rs2, ((uint32_t *)dst->f.i.rt) + 1);
        jne_rj(0);
        int32_t j2 = code_length - 1;
        mov_m32_imm32((void *)(&branch_taken), 0);
        jmp_imm_short(0);
        int32_t je = code_length - 1;
        rj_patch(j1);
        rj_patch(j2);
        mov_m32_imm32((void *)(&branch_taken), 1);
        rj_patch(je);
    }
    else
    {
        int32_t rs1, rs2, rt1, rt2;
        if (!rs_64bit)
        {
            rt1 = allocate_64_register1((uintptr_t)dst->f.i.rt);
            rt2 = allocate_64_register2((uintptr_t)dst->f.i.rt);
            rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
            rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);
        }
        else
        {
            rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
            rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);
            rt1 = allocate_64_register1((uintptr_t)dst->f.i.rt);
            rt2 = allocate_64_register2((uintptr_t)dst->f.i.rt);
        }
        cmp_reg32_reg32(rs1, rt1);
        jne_rj(0);
        int32_t j1 = code_length - 1;
        cmp_reg32_reg32(rs2, rt2);
        jne_rj(0);
        int32_t j2 = code_length - 1;
        mov_m32_imm32((void *)(&branch_taken), 0);
        jmp_imm_short(0);
        int32_t je = code_length - 1;
        rj_patch(j1);
        rj_patch(j2);
        mov_m32_imm32((void *)(&branch_taken), 1);
        rj_patch(je);
    }
}

void genbne()
{
#ifdef INTERPRET_BNE
    gencallinterp((uintptr_t)BNE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BNE, 1);
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
    gencallinterp((uintptr_t)BNE_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BNE_OUT, 1);
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
    gencallinterp((uintptr_t)BNE_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BNE_IDLE, 1);
        return;
    }

    genbne_test();
    gentest_idle();
    genbne();
#endif
}

void genblez_test()
{
    int32_t rs_64bit = is64((uintptr_t)dst->f.i.rs);

    if (!rs_64bit)
    {
        int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);

        cmp_reg32_imm32(rs, 0);
        jg_rj(0);
        int32_t jg = code_length - 1;
        mov_m32_imm32((void *)(&branch_taken), 1);
        jmp_imm_short(0);
        int32_t jend = code_length - 1;
        rj_patch(jg);
        mov_m32_imm32((void *)(&branch_taken), 0);
        rj_patch(jend);
    }
    else if (rs_64bit == -1)
    {
        cmp_m32_imm32(((uint32_t *)dst->f.i.rs) + 1, 0);
        jg_rj(0);
        int32_t jg = code_length - 1;
        jne_rj(0);
        int32_t jn = code_length - 1;
        cmp_m32_imm32((uint32_t *)dst->f.i.rs, 0);
        je_rj(0);
        int32_t je_ = code_length - 1;
        rj_patch(jg);
        mov_m32_imm32((void *)(&branch_taken), 0);
        jmp_imm_short(0);
        int32_t jend = code_length - 1;
        rj_patch(jn);
        rj_patch(je_);
        mov_m32_imm32((void *)(&branch_taken), 1);
        rj_patch(jend);
    }
    else
    {
        int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
        int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);

        cmp_reg32_imm32(rs2, 0);
        jg_rj(0);
        int32_t jg = code_length - 1;
        jne_rj(0);
        int32_t jn = code_length - 1;
        cmp_reg32_imm32(rs1, 0);
        je_rj(0);
        int32_t je_ = code_length - 1;
        rj_patch(jg);
        mov_m32_imm32((void *)(&branch_taken), 0);
        jmp_imm_short(0);
        int32_t jend = code_length - 1;
        rj_patch(jn);
        rj_patch(je_);
        mov_m32_imm32((void *)(&branch_taken), 1);
        rj_patch(jend);
    }
}

void genblez()
{
#ifdef INTERPRET_BLEZ
    gencallinterp((uintptr_t)BLEZ, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLEZ, 1);
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
    gencallinterp((uintptr_t)BLEZ_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLEZ_OUT, 1);
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
    gencallinterp((uintptr_t)BLEZ_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLEZ_IDLE, 1);
        return;
    }

    genblez_test();
    gentest_idle();
    genblez();
#endif
}

void genbgtz_test()
{
    int32_t rs_64bit = is64((uintptr_t)dst->f.i.rs);

    if (!rs_64bit)
    {
        int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);

        cmp_reg32_imm32(rs, 0);
        jle_rj(0);
        int32_t jle = code_length - 1;
        mov_m32_imm32((void *)(&branch_taken), 1);
        jmp_imm_short(0);
        int32_t jend = code_length - 1;
        rj_patch(jle);
        mov_m32_imm32((void *)(&branch_taken), 0);
        rj_patch(jend);
    }
    else if (rs_64bit == -1)
    {
        cmp_m32_imm32(((uint32_t *)dst->f.i.rs) + 1, 0);
        jl_rj(0);
        int32_t jl = code_length - 1;
        jne_rj(0);
        int32_t jn1 = code_length - 1;
        cmp_m32_imm32((uint32_t *)dst->f.i.rs, 0);
        jne_rj(0);
        int32_t jn2 = code_length - 1;
        rj_patch(jl);
        mov_m32_imm32((void *)(&branch_taken), 0);
        jmp_imm_short(0);
        int32_t jend = code_length - 1;
        rj_patch(jn1);
        rj_patch(jn2);
        mov_m32_imm32((void *)(&branch_taken), 1);
        rj_patch(jend);
    }
    else
    {
        int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
        int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);

        cmp_reg32_imm32(rs2, 0);
        jl_rj(0);
        int32_t jl = code_length - 1;
        jne_rj(0);
        int32_t jn1 = code_length - 1;
        cmp_reg32_imm32(rs1, 0);
        jne_rj(0);
        int32_t jn2 = code_length - 1;
        rj_patch(jl);
        mov_m32_imm32((void *)(&branch_taken), 0);
        jmp_imm_short(0);
        int32_t jend = code_length - 1;
        rj_patch(jn1);
        rj_patch(jn2);
        mov_m32_imm32((void *)(&branch_taken), 1);
        rj_patch(jend);
    }
}

void genbgtz()
{
#ifdef INTERPRET_BGTZ
    gencallinterp((uintptr_t)BGTZ, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGTZ, 1);
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
    gencallinterp((uintptr_t)BGTZ_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGTZ_OUT, 1);
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
    gencallinterp((uintptr_t)BGTZ_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGTZ_IDLE, 1);
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
    gencallinterp((uintptr_t)ADDI, 0);
#else
    int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
    int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    mov_reg32_reg32(rt, rs);
    add_reg32_imm32(rt, (int32_t)dst->f.i.immediate);
#endif
}

void genaddiu()
{
#ifdef INTERPRET_ADDIU
    gencallinterp((uintptr_t)ADDIU, 0);
#else
    int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
    int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    mov_reg32_reg32(rt, rs);
    add_reg32_imm32(rt, (int32_t)dst->f.i.immediate);
#endif
}

void genslti()
{
#ifdef INTERPRET_SLTI
    gencallinterp((uintptr_t)SLTI, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);
    int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);
    int64_t imm = (int64_t)dst->f.i.immediate;

    cmp_reg32_imm32(rs2, (uint32_t)(imm >> 32));
    jl_rj(0);
    int32_t jA = code_length - 1;
    jne_rj(0);
    int32_t jB = code_length - 1;
    cmp_reg32_imm32(rs1, (uint32_t)imm);
    jl_rj(0);
    int32_t jC = code_length - 1;
    rj_patch(jB);
    mov_reg32_imm32(rt, 0);
    jmp_imm_short(0);
    int32_t jend = code_length - 1;
    rj_patch(jA);
    rj_patch(jC);
    mov_reg32_imm32(rt, 1);
    rj_patch(jend);
#endif
}

void gensltiu()
{
#ifdef INTERPRET_SLTIU
    gencallinterp((uintptr_t)SLTIU, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);
    int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);
    int64_t imm = (int64_t)dst->f.i.immediate;

    cmp_reg32_imm32(rs2, (uint32_t)(imm >> 32));
    jb_rj(0);
    int32_t jA = code_length - 1;
    jne_rj(0);
    int32_t jB = code_length - 1;
    cmp_reg32_imm32(rs1, (uint32_t)imm);
    jb_rj(0);
    int32_t jC = code_length - 1;
    rj_patch(jB);
    mov_reg32_imm32(rt, 0);
    jmp_imm_short(0);
    int32_t jend = code_length - 1;
    rj_patch(jA);
    rj_patch(jC);
    mov_reg32_imm32(rt, 1);
    rj_patch(jend);
#endif
}

void genandi()
{
#ifdef INTERPRET_ANDI
    gencallinterp((uintptr_t)ANDI, 0);
#else
    int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
    int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    mov_reg32_reg32(rt, rs);
    and_reg32_imm32(rt, (uint16_t)dst->f.i.immediate);
#endif
}

void genori()
{
#ifdef INTERPRET_ORI
    gencallinterp((uintptr_t)ORI, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);
    int32_t rt1 = allocate_64_register1_w((uintptr_t)dst->f.i.rt);
    int32_t rt2 = allocate_64_register2_w((uintptr_t)dst->f.i.rt);

    mov_reg32_reg32(rt1, rs1);
    mov_reg32_reg32(rt2, rs2);
    or_reg32_imm32(rt1, (uint16_t)dst->f.i.immediate);
#endif
}

void genxori()
{
#ifdef INTERPRET_XORI
    gencallinterp((uintptr_t)XORI, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);
    int32_t rt1 = allocate_64_register1_w((uintptr_t)dst->f.i.rt);
    int32_t rt2 = allocate_64_register2_w((uintptr_t)dst->f.i.rt);

    mov_reg32_reg32(rt1, rs1);
    mov_reg32_reg32(rt2, rs2);
    xor_reg32_imm32(rt1, (uint16_t)dst->f.i.immediate);
#endif
}

void genlui()
{
#ifdef INTERPRET_LUI
    gencallinterp((uintptr_t)LUI, 0);
#else
    int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    mov_reg32_imm32(rt, (uint32_t)dst->f.i.immediate << 16);
#endif
}

void gentestl()
{
    uint32_t temp, temp2;

    cmp_m32_imm32((void *)(&branch_taken), 0);
    je_near_rj(0);
    temp = code_length;
    gendelayslot();
    mov_m32_imm32((void *)(&last_addr), dst->addr + (dst - 1)->f.i.immediate * 4);
    gencheck_interrupt((uintptr_t)(dst + (dst - 1)->f.i.immediate));
    jmp(dst->addr + (dst - 1)->f.i.immediate * 4);

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;
    genupdate_count(dst->addr - 4);
    mov_m32_imm32((void *)(&last_addr), dst->addr + 4);
    gencheck_interrupt((uintptr_t)(dst + 1));
    jmp(dst->addr + 4);
}

void genbeql()
{
#ifdef INTERPRET_BEQL
    gencallinterp((uintptr_t)BEQL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BEQL, 1);
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

    cmp_m32_imm32((void *)(&branch_taken), 0);
    je_near_rj(0);
    temp = code_length;
    gendelayslot();
    mov_m32_imm32((void *)(&last_addr), dst->addr + (dst - 1)->f.i.immediate * 4);
    gencheck_interrupt_out(dst->addr + (dst - 1)->f.i.immediate * 4);
    mov_m32_imm32(&jump_to_address, dst->addr + (dst - 1)->f.i.immediate * 4);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_reg64_imm64(EAX, (uintptr_t)jump_to_func);
    call_reg64(EAX);

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;
    genupdate_count(dst->addr - 4);
    mov_m32_imm32((void *)(&last_addr), dst->addr + 4);
    gencheck_interrupt((uintptr_t)(dst + 1));
    jmp(dst->addr + 4);
}

void genbeql_out()
{
#ifdef INTERPRET_BEQL_OUT
    gencallinterp((uintptr_t)BEQL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BEQL_OUT, 1);
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
    gencallinterp((uintptr_t)BEQL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BEQL_IDLE, 1);
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
    gencallinterp((uintptr_t)BNEL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BNEL, 1);
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
    gencallinterp((uintptr_t)BNEL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BNEL_OUT, 1);
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
    gencallinterp((uintptr_t)BNEL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BNEL_IDLE, 1);
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
    gencallinterp((uintptr_t)BLEZL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLEZL, 1);
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
    gencallinterp((uintptr_t)BLEZL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLEZL_OUT, 1);
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
    gencallinterp((uintptr_t)BLEZL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLEZL_IDLE, 1);
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
    gencallinterp((uintptr_t)BGTZL, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGTZL, 1);
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
    gencallinterp((uintptr_t)BGTZL_OUT, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGTZL_OUT, 1);
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
    gencallinterp((uintptr_t)BGTZL_IDLE, 1);
#else
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGTZL_IDLE, 1);
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
    gencallinterp((uintptr_t)DADDI, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);
    int32_t rt1 = allocate_64_register1_w((uintptr_t)dst->f.i.rt);
    int32_t rt2 = allocate_64_register2_w((uintptr_t)dst->f.i.rt);

    mov_reg32_reg32(rt1, rs1);
    mov_reg32_reg32(rt2, rs2);
    add_reg32_imm32(rt1, dst->f.i.immediate);
    adc_reg32_imm32(rt2, (int32_t)dst->f.i.immediate >> 31);
#endif
}

void gendaddiu()
{
#ifdef INTERPRET_DADDIU
    gencallinterp((uintptr_t)DADDIU, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.i.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.i.rs);
    int32_t rt1 = allocate_64_register1_w((uintptr_t)dst->f.i.rt);
    int32_t rt2 = allocate_64_register2_w((uintptr_t)dst->f.i.rt);

    mov_reg32_reg32(rt1, rs1);
    mov_reg32_reg32(rt2, rs2);
    add_reg32_imm32(rt1, dst->f.i.immediate);
    adc_reg32_imm32(rt2, (int32_t)dst->f.i.immediate >> 31);
#endif
}

void genldl()
{
    gencallinterp((uintptr_t)LDL, 0);
}

void genldr()
{
    gencallinterp((uintptr_t)LDR, 0);
}

void genlb()
{
#ifdef INTERPRET_LB
    gencallinterp((uintptr_t)LB, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t *)dst->f.i.rs);
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uint64_t)readmemb);
        cmp_reg32_imm32(EAX, (uintptr_t)read_rdramb);
    }
    je_near_rj(0);
    int32_t jhit = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_m64_imm64((void *)(&rdword), (uintptr_t)dst->f.i.rt);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4_r10(EBX, EBX, (uint64_t)readmemb);
    call_reg64(EBX);
    movsx_reg32_m8(EAX, (unsigned char *)dst->f.i.rt);
    jmp_imm_short(0);
    int32_t jdone = code_length - 1;

    rj_patch_near(jhit);
    and_reg32_imm32(EBX, 0x7FFFFF);
    xor_reg8_imm8(BL, 3);
    movsx_reg32_8preg32pimm32_r11(EAX, EBX, (uintptr_t)rdram);
    rj_patch(jdone);

    set_register_state(EAX, (uintptr_t)dst->f.i.rt, 1);
#endif
}

void genlh()
{
#ifdef INTERPRET_LH
    gencallinterp((uintptr_t)LH, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t *)dst->f.i.rs);
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uint64_t)readmemh);
        cmp_reg32_imm32(EAX, (uintptr_t)read_rdramh);
    }
    je_near_rj(0);
    int32_t jhit = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_m64_imm64((void *)(&rdword), (uintptr_t)dst->f.i.rt);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4_r10(EBX, EBX, (uint64_t)readmemh);
    call_reg64(EBX);
    movsx_reg32_m16(EAX, (uint16_t *)dst->f.i.rt);
    jmp_imm_short(0);
    int32_t jdone = code_length - 1;

    rj_patch_near(jhit);
    and_reg32_imm32(EBX, 0x7FFFFF);
    xor_reg8_imm8(BL, 2);
    movsx_reg32_16preg32pimm32_r11(EAX, EBX, (uintptr_t)rdram);
    rj_patch(jdone);

    set_register_state(EAX, (uintptr_t)dst->f.i.rt, 1);
#endif
}

void genlwl()
{
    gencallinterp((uintptr_t)LWL, 0);
}

void genlw()
{
#ifdef INTERPRET_LW
    gencallinterp((uintptr_t)LW, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t *)dst->f.i.rs);
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uint64_t)readmem);
        cmp_reg32_imm32(EAX, (uintptr_t)read_rdram);
    }
    je_near_rj(0);
    int32_t jhit = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_m64_imm64((void *)(&rdword), (uintptr_t)dst->f.i.rt);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4_r10(EBX, EBX, (uint64_t)readmem);
    call_reg64(EBX);
    mov_eax_memoffs32((uint32_t *)(dst->f.i.rt));
    jmp_imm_short(0);
    int32_t jdone = code_length - 1;

    rj_patch_near(jhit);
    and_reg32_imm32(EBX, 0x7FFFFF);
    mov_reg32_preg32pimm32_r11(EAX, EBX, (uintptr_t)rdram);
    rj_patch(jdone);

    set_register_state(EAX, (uintptr_t)dst->f.i.rt, 1);
#endif
}

void genlbu()
{
#ifdef INTERPRET_LBU
    gencallinterp((uintptr_t)LBU, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t *)dst->f.i.rs);
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uint64_t)readmemb);
        cmp_reg32_imm32(EAX, (uintptr_t)read_rdramb);
    }
    je_near_rj(0);
    int32_t jhit = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_m64_imm64((void *)(&rdword), (uintptr_t)dst->f.i.rt);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4pimm64(EBX, EBX, (uintptr_t)readmemb);
    call_reg64(EBX);
    mov_reg32_m32(EAX, (uint32_t *)dst->f.i.rt);
    jmp_imm_short(0);
    int32_t jdone = code_length - 1;

    rj_patch_near(jhit);
    and_reg32_imm32(EBX, 0x7FFFFF);
    xor_reg8_imm8(BL, 3);
    mov_reg32_preg32pimm32_r11(EAX, EBX, (uintptr_t)rdram);
    rj_patch(jdone);

    and_eax_imm32(0xFF);

    set_register_state(EAX, (uintptr_t)dst->f.i.rt, 1);
#endif
}

void genlhu()
{
#ifdef INTERPRET_LHU
    gencallinterp((uintptr_t)LHU, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t *)dst->f.i.rs);
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uintptr_t)readmemh);
        cmp_reg32_imm32(EAX, (uintptr_t)read_rdramh);
    }
    je_near_rj(0);
    int32_t jhit = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_m64_imm64((void *)(&rdword), (uintptr_t)dst->f.i.rt);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4pimm64(EBX, EBX, (uintptr_t)readmemh);
    call_reg64(EBX);
    mov_reg32_m32(EAX, (uint32_t *)dst->f.i.rt);
    jmp_imm_short(0);
    int32_t jdone = code_length - 1;

    rj_patch_near(jhit);
    and_reg32_imm32(EBX, 0x7FFFFF);
    xor_reg8_imm8(BL, 2);
    mov_reg32_preg32pimm32_r11(EAX, EBX, (uintptr_t)rdram);
    rj_patch(jdone);

    and_eax_imm32(0xFFFF);

    set_register_state(EAX, (uintptr_t)dst->f.i.rt, 1);
#endif
}

void genlwr()
{
    gencallinterp((uintptr_t)LWR, 0);
}

void genlwu()
{
#ifdef INTERPRET_LWU
    gencallinterp((uintptr_t)LWU, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t *)dst->f.i.rs);
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uintptr_t)readmem);
        cmp_reg32_imm32(EAX, (uintptr_t)read_rdram);
    }
    je_near_rj(0);
    int32_t jhit = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_m64_imm64((void *)(&rdword), (uintptr_t)dst->f.i.rt);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4pimm64(EBX, EBX, (uintptr_t)readmem);
    call_reg64(EBX);
    mov_eax_memoffs32((uint32_t *)(dst->f.i.rt));
    jmp_imm_short(0);
    int32_t jdone = code_length - 1;

    rj_patch_near(jhit);
    and_reg32_imm32(EBX, 0x7FFFFF);
    mov_reg32_preg32pimm32_r11(EAX, EBX, (uintptr_t)rdram);
    rj_patch(jdone);

    xor_reg32_reg32(EBX, EBX);

    set_64_register_state(EAX, EBX, (uintptr_t)dst->f.i.rt, 1);
#endif
}

void gensb()
{
#ifdef INTERPRET_SB
    gencallinterp((uintptr_t)SB, 0);
#else
    free_all_registers();
    simplify_access();
    mov_reg8_m8(CL, (unsigned char *)dst->f.i.rt);
    mov_eax_memoffs32((uint32_t *)dst->f.i.rs);
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uintptr_t)writememb);
        cmp_reg32_imm32(EAX, (uintptr_t)write_rdramb);
    }
    je_near_rj(0);
    int32_t jhit = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_m8_reg8((unsigned char *)(&g_byte), CL);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4pimm64(EBX, EBX, (uintptr_t)writememb);
    call_reg64(EBX);
    mov_eax_memoffs32((uint32_t *)(&address));
    jmp_imm_short(0);
    int32_t jstore_done = code_length - 1;

    rj_patch_near(jhit);
    mov_reg32_reg32(EAX, EBX);
    and_reg32_imm32(EBX, 0x7FFFFF);
    xor_reg8_imm8(BL, 3);
    mov_preg32pimm32_reg8_r11(EBX, (uintptr_t)rdram, CL);

    rj_patch(jstore_done);
    // EAX holds the store's target address here. Do the self-modifying-code
    // invalidation check in C (no 64-bit pointer truncation).
    gen_check_invalidate();
#endif
}

void gensh()
{
#ifdef INTERPRET_SH
    gencallinterp((uintptr_t)SH, 0);
#else
    free_all_registers();
    simplify_access();
    mov_reg16_m16(CX, (uint16_t *)dst->f.i.rt);
    mov_eax_memoffs32((uint32_t *)dst->f.i.rs);
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uintptr_t)writememh);
        cmp_reg32_imm32(EAX, (uintptr_t)write_rdramh);
    }
    je_near_rj(0);
    int32_t jhit = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_m16_reg16((uint16_t *)(&hword), CX);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4pimm64(EBX, EBX, (uintptr_t)writememh);
    call_reg64(EBX);
    mov_eax_memoffs32((uint32_t *)(&address));
    jmp_imm_short(0);
    int32_t jstore_done = code_length - 1;

    rj_patch_near(jhit);
    mov_reg32_reg32(EAX, EBX);
    and_reg32_imm32(EBX, 0x7FFFFF);
    xor_reg8_imm8(BL, 2);
    mov_preg32pimm32_reg16_r11(EBX, (uintptr_t)rdram, CX);

    rj_patch(jstore_done);
    // EAX holds the store's target address here. Do the self-modifying-code
    // invalidation check in C (no 64-bit pointer truncation).
    gen_check_invalidate();
#endif
}

void genswl()
{
    gencallinterp((uintptr_t)SWL, 0);
}

void gensw()
{
#ifdef INTERPRET_SW
    gencallinterp((uintptr_t)SW, 0);
#else
    free_all_registers();
    simplify_access();
    mov_reg32_m32(ECX, (uint32_t *)dst->f.i.rt);
    mov_eax_memoffs32((uint32_t *)dst->f.i.rs);
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uintptr_t)writemem);
        cmp_reg32_imm32(EAX, (uintptr_t)write_rdram);
    }
    je_near_rj(0);
    int32_t jfast = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_m32_reg32((void *)(&word), ECX);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4pimm64(EBX, EBX, (uintptr_t)writemem);
    call_reg64(EBX);
    mov_eax_memoffs32((uint32_t *)(&address));
    jmp_imm_short(0);
    int32_t jafter = code_length - 1;

    rj_patch_near(jfast);
    mov_reg32_reg32(EAX, EBX);
    and_reg32_imm32(EBX, 0x7FFFFF);
    mov_preg32pimm32_reg32_r11(EBX, (uintptr_t)rdram, ECX);

    rj_patch(jafter);
    // EAX holds the store's target address here. Do the self-modifying-code
    // invalidation check in C (no 64-bit pointer truncation).
    gen_check_invalidate();
#endif
}

void gensdl()
{
    gencallinterp((uintptr_t)SDL, 0);
}

void gensdr()
{
    gencallinterp((uintptr_t)SDR, 0);
}

void genswr()
{
    gencallinterp((uintptr_t)SWR, 0);
}

#include <malloc.h>

inline void put8gr(unsigned char octet)
{
    (*inst_pointer)[code_length] = octet;
    code_length++;
    if (code_length == max_code_length)
    {
        // Must use realloc_exec, not plain realloc: the buffer was allocated via
        // malloc_exec (VirtualAlloc, PAGE_EXECUTE_READWRITE). Plain realloc would
        // move it to non-executable heap. Copy the full old allocation so the
        // buffer tail is preserved (see grow_buffer in Assemble.cpp).
        size_t old_size = max_code_length;
        max_code_length += JUMP_TABLE_SIZE;
        *inst_pointer = (unsigned char *)realloc_exec(*inst_pointer, old_size, max_code_length);
    }
}

void gencheck_cop1_unusable()
{
    uint32_t temp, temp2;
    free_all_registers();
    simplify_access();
    test_m32_imm32((uint32_t *)&core_Status, 0x20000000);
    jne_rj(0);
    temp = code_length;

    gencallinterp((uintptr_t)check_cop1_unusable, 0);

    temp2 = code_length;
    code_length = temp - 1;
    put8gr(temp2 - temp);
    code_length = temp2;
}

void genlwc1()
{
#ifdef INTERPRET_LWC1
    gencallinterp((uintptr_t)LWC1, 0);
#else
    gencheck_cop1_unusable();

    mov_eax_memoffs32((uint32_t *)(&reg[dst->f.lf.base]));
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uintptr_t)readmem);
        cmp_reg32_imm32(EAX, (uintptr_t)read_rdram);
    }
    je_near_rj(0);
    int32_t jfast = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_reg64_m64(EDX, (void *)(&reg_cop1_simple[dst->f.lf.ft])); // 64-bit: this is a float* pointer
    mov_m64_reg64((void *)(&rdword), EDX);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4pimm64(EBX, EBX, (uintptr_t)readmem);
    call_reg64(EBX);
    jmp_imm_short(0);
    int32_t jend = code_length - 1;

    rj_patch_near(jfast);
    and_reg32_imm32(EBX, 0x7FFFFF);
    mov_reg32_preg32pimm32_r11(EAX, EBX, (uintptr_t)rdram);
    mov_reg64_m64(EBX, (void *)(&reg_cop1_simple[dst->f.lf.ft])); // 64-bit: this is a float* pointer
    mov_preg32_reg32(EBX, EAX);
    rj_patch(jend);
#endif
}

void genldc1()
{
#ifdef INTERPRET_LDC1
    gencallinterp((uintptr_t)LDC1, 0);
#else
    gencheck_cop1_unusable();

    mov_eax_memoffs32((uint32_t *)(&reg[dst->f.lf.base]));
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uintptr_t)readmemd);
        cmp_reg32_imm32(EAX, (uintptr_t)read_rdramd);
    }
    je_near_rj(0);
    int32_t jfast = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_reg64_m64(EDX, (void *)(&reg_cop1_double[dst->f.lf.ft])); // 64-bit: this is a double* pointer
    mov_m64_reg64((void *)(&rdword), EDX);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4pimm64(EBX, EBX, (uintptr_t)readmemd);
    call_reg64(EBX);
    jmp_imm_short(0);
    int32_t jend = code_length - 1;

    rj_patch_near(jfast);
    and_reg32_imm32(EBX, 0x7FFFFF);
    mov_reg32_preg32pimm32_r11(EAX, EBX, (uintptr_t)rdram + 4);
    mov_reg32_preg32pimm32_r11(ECX, EBX, (uintptr_t)rdram);
    mov_reg64_m64(EBX, (void *)(&reg_cop1_double[dst->f.lf.ft])); // 64-bit: this is a double* pointer
    mov_preg32_reg32(EBX, EAX);
    mov_preg32pimm32_reg32(EBX, 4, ECX);
    rj_patch(jend);
#endif
}

void gencache()
{
}

void genld()
{
#ifdef INTERPRET_LD
    gencallinterp((uintptr_t)LD, 0);
#else
    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t *)dst->f.i.rs);
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uintptr_t)readmemd);
        cmp_reg32_imm32(EAX, (uintptr_t)read_rdramd);
    }
    je_near_rj(0);
    int32_t jfast = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_m64_imm64((void *)(&rdword), (uintptr_t)dst->f.i.rt);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4pimm64(EBX, EBX, (uintptr_t)readmemd);
    call_reg64(EBX);
    mov_eax_memoffs32((uint32_t *)(dst->f.i.rt));
    mov_reg32_m32(ECX, (uint32_t *)(dst->f.i.rt) + 1);
    jmp_imm_short(0);
    int32_t jcommon = code_length - 1;

    rj_patch_near(jfast);
    and_reg32_imm32(EBX, 0x7FFFFF);
    mov_reg32_preg32pimm32_r11(EAX, EBX, (uintptr_t)rdram + 4);
    mov_reg32_preg32pimm32_r11(ECX, EBX, (uintptr_t)rdram);

    rj_patch(jcommon);
    set_64_register_state(EAX, ECX, (uintptr_t)dst->f.i.rt, 1);
#endif
}

void genswc1()
{
#ifdef INTERPRET_SWC1
    gencallinterp((uintptr_t)SWC1, 0);
#else
    gencheck_cop1_unusable();

    mov_reg64_m64(EDX, (void *)(&reg_cop1_simple[dst->f.lf.ft])); // 64-bit: this is a float* pointer
    mov_reg32_preg32(ECX, EDX);
    mov_eax_memoffs32((uint32_t *)(&reg[dst->f.lf.base]));
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uintptr_t)writemem);
        cmp_reg32_imm32(EAX, (uintptr_t)write_rdram);
    }
    je_near_rj(0);
    int32_t jfast = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_m32_reg32((void *)(&word), ECX);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4pimm64(EBX, EBX, (uintptr_t)writemem);
    call_reg64(EBX);
    mov_eax_memoffs32((uint32_t *)(&address));
    jmp_imm_short(0);
    int32_t jafter = code_length - 1;

    rj_patch_near(jfast);
    mov_reg32_reg32(EAX, EBX);
    and_reg32_imm32(EBX, 0x7FFFFF);
    mov_preg32pimm32_reg32_r11(EBX, (uintptr_t)rdram, ECX);

    rj_patch(jafter);
    // EAX holds the store's target address here. Do the self-modifying-code
    // invalidation check in C (no 64-bit pointer truncation).
    gen_check_invalidate();
#endif
}

void gensdc1()
{
#ifdef INTERPRET_SDC1
    gencallinterp((uintptr_t)SDC1, 0);
#else
    gencheck_cop1_unusable();

    mov_reg64_m64(ESI, (void *)(&reg_cop1_double[dst->f.lf.ft])); // 64-bit: this is a double* pointer
    mov_reg32_preg32(ECX, ESI);
    mov_reg32_preg32pimm32(EDX, ESI, 4);
    mov_eax_memoffs32((uint32_t *)(&reg[dst->f.lf.base]));
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uintptr_t)writememd);
        cmp_reg32_imm32(EAX, (uintptr_t)write_rdramd);
    }
    je_near_rj(0);
    int32_t jfast = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_m32_reg32((void *)(&dword), ECX);
    mov_m32_reg32((uint32_t *)(&dword) + 1, EDX);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4pimm64(EBX, EBX, (uint64_t)writememd);
    call_reg64(EBX);
    mov_eax_memoffs32((uint32_t *)(&address));
    jmp_imm_short(0);
    int32_t jafter = code_length - 1;

    rj_patch_near(jfast);
    mov_reg32_reg32(EAX, EBX);
    and_reg32_imm32(EBX, 0x7FFFFF);
    mov_preg32pimm32_reg32_r11(EBX, (uintptr_t)rdram + 4, ECX);
    mov_preg32pimm32_reg32_r11(EBX, (uintptr_t)rdram, EDX);

    rj_patch(jafter);
    // EAX holds the store's target address here. Do the self-modifying-code
    // invalidation check in C (no 64-bit pointer truncation).
    gen_check_invalidate();
#endif
}

void gensd()
{
#ifdef INTERPRET_SD
    gencallinterp((uintptr_t)SD, 0);
#else
    free_all_registers();
    simplify_access();

    mov_reg32_m32(ECX, (uint32_t *)dst->f.i.rt);
    mov_reg32_m32(EDX, ((uint32_t *)dst->f.i.rt) + 1);
    mov_eax_memoffs32((uint32_t *)dst->f.i.rs);
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
        mov_reg32_preg32x4pimm64(EAX, EAX, (uint64_t)writememd);
        cmp_reg32_imm32(EAX, (uintptr_t)write_rdramd);
    }
    je_near_rj(0);
    int32_t jfast = code_length - 4;

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_m32_reg32((void *)(&address), EBX);
    mov_m32_reg32((void *)(&dword), ECX);
    mov_m32_reg32((uint32_t *)(&dword) + 1, EDX);
    shr_reg32_imm8(EBX, 16);
    mov_reg32_preg32x4pimm64(EBX, EBX, (uint64_t)writememd);
    call_reg64(EBX);
    mov_eax_memoffs32((uint32_t *)(&address));
    jmp_imm_short(0);
    int32_t jafter = code_length - 1;

    rj_patch_near(jfast);
    mov_reg32_reg32(EAX, EBX);
    and_reg32_imm32(EBX, 0x7FFFFF);
    mov_preg32pimm32_reg32_r11(EBX, (uintptr_t)rdram + 4, ECX);
    mov_preg32pimm32_reg32_r11(EBX, (uintptr_t)rdram, EDX);

    rj_patch(jafter);
    // EAX holds the store's target address here. Do the self-modifying-code
    // invalidation check in C (no 64-bit pointer truncation).
    gen_check_invalidate();
#endif
}

void genll()
{
    gencallinterp((uintptr_t)LL, 0);
}

void gensc()
{
    gencallinterp((uintptr_t)SC, 0);
}
