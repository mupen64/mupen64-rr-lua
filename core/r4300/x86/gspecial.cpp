/**
 * Mupen64 - gspecial.c
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
#include <shared/services/LuaService.h>
#include <stdio.h>
#include "../exception.h"
#include "../macros.h"
#include "../ops.h"
#include "../r4300.h"
#include "../recomp.h"
#include "../recomph.h"
#include "assemble.h"
#include "interpret.h"

void gensll()
{
#ifdef INTERPRET_SLL
    gencallinterp((uint32_t)SLL, 0);
#else
    int32_t rt = allocate_register((uint32_t*)dst->f.r.rt);
    int32_t rd = allocate_register_w((uint32_t*)dst->f.r.rd);

    mov_reg32_reg32(rd, rt);
    shl_reg32_imm8(rd, dst->f.r.sa);
#endif
}

void gensrl()
{
#ifdef INTERPRET_SRL
    gencallinterp((uint32_t)SRL, 0);
#else
    int32_t rt = allocate_register((uint32_t*)dst->f.r.rt);
    int32_t rd = allocate_register_w((uint32_t*)dst->f.r.rd);

    mov_reg32_reg32(rd, rt);
    shr_reg32_imm8(rd, dst->f.r.sa);
#endif
}

void gensra()
{
#ifdef INTERPRET_SRA
    gencallinterp((uint32_t)SRA, 0);
#else
    int32_t rt = allocate_register((uint32_t*)dst->f.r.rt);
    int32_t rd = allocate_register_w((uint32_t*)dst->f.r.rd);

    mov_reg32_reg32(rd, rt);
    sar_reg32_imm8(rd, dst->f.r.sa);
#endif
}

void gensllv()
{
#ifdef INTERPRET_SLLV
    gencallinterp((uint32_t)SLLV, 0);
#else
    int32_t rt, rd;
    allocate_register_manually(ECX, (uint32_t*)dst->f.r.rs);

    rt = allocate_register((uint32_t*)dst->f.r.rt);
    rd = allocate_register_w((uint32_t*)dst->f.r.rd);

    if (rd != ECX)
    {
        mov_reg32_reg32(rd, rt);
        shl_reg32_cl(rd);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rt);
        shl_reg32_cl(temp);
        mov_reg32_reg32(rd, temp);
    }
#endif
}

void gensrlv()
{
#ifdef INTERPRET_SRLV
    gencallinterp((uint32_t)SRLV, 0);
#else
    int32_t rt, rd;
    allocate_register_manually(ECX, (uint32_t*)dst->f.r.rs);

    rt = allocate_register((uint32_t*)dst->f.r.rt);
    rd = allocate_register_w((uint32_t*)dst->f.r.rd);

    if (rd != ECX)
    {
        mov_reg32_reg32(rd, rt);
        shr_reg32_cl(rd);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rt);
        shr_reg32_cl(temp);
        mov_reg32_reg32(rd, temp);
    }
#endif
}

void gensrav()
{
#ifdef INTERPRET_SRAV
    gencallinterp((uint32_t)SRAV, 0);
#else
    int32_t rt, rd;
    allocate_register_manually(ECX, (uint32_t*)dst->f.r.rs);

    rt = allocate_register((uint32_t*)dst->f.r.rt);
    rd = allocate_register_w((uint32_t*)dst->f.r.rd);

    if (rd != ECX)
    {
        mov_reg32_reg32(rd, rt);
        sar_reg32_cl(rd);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rt);
        sar_reg32_cl(temp);
        mov_reg32_reg32(rd, temp);
    }
#endif
}

void genjr()
{
#ifdef INTERPRET_JR
    gencallinterp((uint32_t)JR, 1);
#else
    static uint32_t precomp_instr_size = sizeof(precomp_instr);
    uint32_t diff = (uint32_t)(&dst->local_addr) - (uint32_t)(dst);
    uint32_t diff_need = (uint32_t)(&dst->reg_cache_infos.need_map) - (uint32_t)(dst);
    uint32_t diff_wrap = (uint32_t)(&dst->reg_cache_infos.jump_wrapper) - (uint32_t)(dst);
    uint32_t temp, temp2;

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)JR, 1);
        return;
    }

    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t*)dst->f.i.rs);
    mov_memoffs32_eax((uint32_t*)&local_rs);

    gendelayslot();

    mov_eax_memoffs32((uint32_t*)&local_rs);
    mov_memoffs32_eax((uint32_t*)&last_addr);

    gencheck_interrupt_reg();

    mov_eax_memoffs32((uint32_t*)&local_rs);
    mov_reg32_reg32(EBX, EAX);
    and_eax_imm32(0xFFFFF000);
    cmp_eax_imm32(dst_block->start & 0xFFFFF000);
    je_near_rj(0);
    temp = code_length;

    mov_m32_reg32(&jump_to_address, EBX);
    mov_m32_imm32((uint32_t*)(&PC), (uint32_t)(dst + 1));
    mov_reg32_imm32(EAX, (uint32_t)jump_to_func);
    call_reg32(EAX);

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;

    mov_reg32_reg32(EAX, EBX);
    sub_eax_imm32(dst_block->start);
    shr_reg32_imm8(EAX, 2);
    mul_m32((uint32_t*)(&precomp_instr_size));

    mov_reg32_preg32pimm32(EBX, EAX, (uint32_t)(dst_block->block) + diff_need);
    cmp_reg32_imm32(EBX, 1);
    jne_rj(7);

    add_eax_imm32((uint32_t)(dst_block->block) + diff_wrap); // 5
    jmp_reg32(EAX); // 2

    mov_reg32_preg32pimm32(EAX, EAX, (uint32_t)(dst_block->block) + diff);
    add_reg32_m32(EAX, (uint32_t*)(&dst_block->code));

    jmp_reg32(EAX);
#endif
}

void genjalr()
{
#ifdef INTERPRET_JALR
    gencallinterp((uint32_t)JALR, 0);
#else
    static uint32_t precomp_instr_size = sizeof(precomp_instr);
    uint32_t diff = (uint32_t)(&dst->local_addr) - (uint32_t)(dst);
    uint32_t diff_need = (uint32_t)(&dst->reg_cache_infos.need_map) - (uint32_t)(dst);
    uint32_t diff_wrap = (uint32_t)(&dst->reg_cache_infos.jump_wrapper) - (uint32_t)(dst);
    uint32_t temp, temp2;

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) || !g_config.is_compiled_jump_enabled)
    {
        gencallinterp((uint32_t)JALR, 1);
        return;
    }

    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t*)dst->f.r.rs);
    mov_memoffs32_eax((uint32_t*)&local_rs);

    gendelayslot();

    mov_m32_imm32((uint32_t*)(dst - 1)->f.r.rd, dst->addr + 4);
    if ((dst->addr + 4) & 0x80000000)
        mov_m32_imm32(((uint32_t*)(dst - 1)->f.r.rd) + 1, 0xFFFFFFFF);
    else
        mov_m32_imm32(((uint32_t*)(dst - 1)->f.r.rd) + 1, 0);

    mov_eax_memoffs32((uint32_t*)&local_rs);
    mov_memoffs32_eax((uint32_t*)&last_addr);

    gencheck_interrupt_reg();

    mov_eax_memoffs32((uint32_t*)&local_rs);
    mov_reg32_reg32(EBX, EAX);
    and_eax_imm32(0xFFFFF000);
    cmp_eax_imm32(dst_block->start & 0xFFFFF000);
    je_near_rj(0);
    temp = code_length;

    mov_m32_reg32(&jump_to_address, EBX);
    mov_m32_imm32((uint32_t*)(&PC), (uint32_t)(dst + 1));
    mov_reg32_imm32(EAX, (uint32_t)jump_to_func);
    call_reg32(EAX);

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;

    mov_reg32_reg32(EAX, EBX);
    sub_eax_imm32(dst_block->start);
    shr_reg32_imm8(EAX, 2);
    mul_m32((uint32_t*)(&precomp_instr_size));

    mov_reg32_preg32pimm32(EBX, EAX, (uint32_t)(dst_block->block) + diff_need);
    cmp_reg32_imm32(EBX, 1);
    jne_rj(7);

    add_eax_imm32((uint32_t)(dst_block->block) + diff_wrap); // 5
    jmp_reg32(EAX); // 2

    mov_reg32_preg32pimm32(EAX, EAX, (uint32_t)(dst_block->block) + diff);
    add_reg32_m32(EAX, (uint32_t*)(&dst_block->code));

    jmp_reg32(EAX);
#endif
}

void gensyscall()
{
#ifdef INTERPRET_SYSCALL
    gencallinterp((uint32_t)SYSCALL, 0);
#else
    free_all_registers();
    simplify_access();
    mov_m32_imm32(&core_Cause, 8 << 2);
    gencallinterp((uint32_t)exception_general, 0);
#endif
}

void gensync()
{
#ifdef LUA_BREAKPOINTSYNC_DYNA

#endif
}

void genmfhi()
{
#ifdef INTERPRET_MFHI
    gencallinterp((uint32_t)MFHI, 0);
#else
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);
    int32_t hi1 = allocate_64_register1((uint32_t*)&hi);
    int32_t hi2 = allocate_64_register2((uint32_t*)&hi);

    mov_reg32_reg32(rd1, hi1);
    mov_reg32_reg32(rd2, hi2);
#endif
}

void genmthi()
{
#ifdef INTERPRET_MTHI
    gencallinterp((uint32_t)MTHI, 0);
#else
    int32_t hi1 = allocate_64_register1_w((uint32_t*)&hi);
    int32_t hi2 = allocate_64_register2_w((uint32_t*)&hi);
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.r.rs);

    mov_reg32_reg32(hi1, rs1);
    mov_reg32_reg32(hi2, rs2);
#endif
}

void genmflo()
{
#ifdef INTERPRET_MFLO
    gencallinterp((uint32_t)MFLO, 0);
#else
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);
    int32_t lo1 = allocate_64_register1((uint32_t*)&lo);
    int32_t lo2 = allocate_64_register2((uint32_t*)&lo);

    mov_reg32_reg32(rd1, lo1);
    mov_reg32_reg32(rd2, lo2);
#endif
}

void genmtlo()
{
#ifdef INTERPRET_MTLO
    gencallinterp((uint32_t)MTLO, 0);
#else
    int32_t lo1 = allocate_64_register1_w((uint32_t*)&lo);
    int32_t lo2 = allocate_64_register2_w((uint32_t*)&lo);
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.r.rs);

    mov_reg32_reg32(lo1, rs1);
    mov_reg32_reg32(lo2, rs2);
#endif
}

void gendsllv()
{
#ifdef INTERPRET_DSLLV
    gencallinterp((uint32_t)DSLLV, 0);
#else
    int32_t rt1, rt2, rd1, rd2;
    allocate_register_manually(ECX, (uint32_t*)dst->f.r.rs);

    rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    if (rd1 != ECX && rd2 != ECX)
    {
        mov_reg32_reg32(rd1, rt1);
        mov_reg32_reg32(rd2, rt2);
        shld_reg32_reg32_cl(rd2, rd1);
        shl_reg32_cl(rd1);
        test_reg32_imm32(ECX, 0x20);
        je_rj(4);
        mov_reg32_reg32(rd2, rd1); // 2
        xor_reg32_reg32(rd1, rd1); // 2
    }
    else
    {
        int32_t temp1, temp2;
        force_32(ECX);
        temp1 = lru_register();
        temp2 = lru_register_exc1(temp1);
        free_register(temp1);
        free_register(temp2);

        mov_reg32_reg32(temp1, rt1);
        mov_reg32_reg32(temp2, rt2);
        shld_reg32_reg32_cl(temp2, temp1);
        shl_reg32_cl(temp1);
        test_reg32_imm32(ECX, 0x20);
        je_rj(4);
        mov_reg32_reg32(temp2, temp1); // 2
        xor_reg32_reg32(temp1, temp1); // 2

        mov_reg32_reg32(rd1, temp1);
        mov_reg32_reg32(rd2, temp2);
    }
#endif
}

void gendsrlv()
{
#ifdef INTERPRET_DSRLV
    gencallinterp((uint32_t)DSRLV, 0);
#else
    int32_t rt1, rt2, rd1, rd2;
    allocate_register_manually(ECX, (uint32_t*)dst->f.r.rs);

    rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    if (rd1 != ECX && rd2 != ECX)
    {
        mov_reg32_reg32(rd1, rt1);
        mov_reg32_reg32(rd2, rt2);
        shrd_reg32_reg32_cl(rd1, rd2);
        shr_reg32_cl(rd2);
        test_reg32_imm32(ECX, 0x20);
        je_rj(4);
        mov_reg32_reg32(rd1, rd2); // 2
        xor_reg32_reg32(rd2, rd2); // 2
    }
    else
    {
        int32_t temp1, temp2;
        force_32(ECX);
        temp1 = lru_register();
        temp2 = lru_register_exc1(temp1);
        free_register(temp1);
        free_register(temp2);

        mov_reg32_reg32(temp1, rt1);
        mov_reg32_reg32(temp2, rt2);
        shrd_reg32_reg32_cl(temp1, temp2);
        shr_reg32_cl(temp2);
        test_reg32_imm32(ECX, 0x20);
        je_rj(4);
        mov_reg32_reg32(temp1, temp2); // 2
        xor_reg32_reg32(temp2, temp2); // 2

        mov_reg32_reg32(rd1, temp1);
        mov_reg32_reg32(rd2, temp2);
    }
#endif
}

void gendsrav()
{
#ifdef INTERPRET_DSRAV
    gencallinterp((uint32_t)DSRAV, 0);
#else
    int32_t rt1, rt2, rd1, rd2;
    allocate_register_manually(ECX, (uint32_t*)dst->f.r.rs);

    rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    if (rd1 != ECX && rd2 != ECX)
    {
        mov_reg32_reg32(rd1, rt1);
        mov_reg32_reg32(rd2, rt2);
        shrd_reg32_reg32_cl(rd1, rd2);
        sar_reg32_cl(rd2);
        test_reg32_imm32(ECX, 0x20);
        je_rj(5);
        mov_reg32_reg32(rd1, rd2); // 2
        sar_reg32_imm8(rd2, 31); // 3
    }
    else
    {
        int32_t temp1, temp2;
        force_32(ECX);
        temp1 = lru_register();
        temp2 = lru_register_exc1(temp1);
        free_register(temp1);
        free_register(temp2);

        mov_reg32_reg32(temp1, rt1);
        mov_reg32_reg32(temp2, rt2);
        shrd_reg32_reg32_cl(temp1, temp2);
        sar_reg32_cl(temp2);
        test_reg32_imm32(ECX, 0x20);
        je_rj(5);
        mov_reg32_reg32(temp1, temp2); // 2
        sar_reg32_imm8(temp2, 31); // 3

        mov_reg32_reg32(rd1, temp1);
        mov_reg32_reg32(rd2, temp2);
    }
#endif
}

void genmult()
{
#ifdef INTERPRET_MULT
    gencallinterp((uint32_t)MULT, 0);
#else
    int32_t rs, rt;
    allocate_register_manually_w(EAX, (uint32_t*)&lo, 0);
    allocate_register_manually_w(EDX, (uint32_t*)&hi, 0);
    rs = allocate_register((uint32_t*)dst->f.r.rs);
    rt = allocate_register((uint32_t*)dst->f.r.rt);
    mov_reg32_reg32(EAX, rs);
    imul_reg32(rt);
#endif
}

void genmultu()
{
#ifdef INTERPRET_MULTU
    gencallinterp((uint32_t)MULTU, 0);
#else
    int32_t rs, rt;
    allocate_register_manually_w(EAX, (uint32_t*)&lo, 0);
    allocate_register_manually_w(EDX, (uint32_t*)&hi, 0);
    rs = allocate_register((uint32_t*)dst->f.r.rs);
    rt = allocate_register((uint32_t*)dst->f.r.rt);
    mov_reg32_reg32(EAX, rs);
    mul_reg32(rt);
#endif
}

void gendiv()
{
#ifdef INTERPRET_DIV
    gencallinterp((uint32_t)DIV, 0);
#else
    int32_t rs, rt;
    allocate_register_manually_w(EAX, (uint32_t*)&lo, 0);
    allocate_register_manually_w(EDX, (uint32_t*)&hi, 0);
    rs = allocate_register((uint32_t*)dst->f.r.rs);
    rt = allocate_register((uint32_t*)dst->f.r.rt);
    cmp_reg32_imm32(rt, 0);
    je_rj((rs == EAX ? 0 : 2) + 1 + 2);
    mov_reg32_reg32(EAX, rs); // 0 or 2
    cdq(); // 1
    idiv_reg32(rt); // 2
#endif
}

void gendivu()
{
#ifdef INTERPRET_DIVU
    gencallinterp((uint32_t)DIVU, 0);
#else
    int32_t rs, rt;
    allocate_register_manually_w(EAX, (uint32_t*)&lo, 0);
    allocate_register_manually_w(EDX, (uint32_t*)&hi, 0);
    rs = allocate_register((uint32_t*)dst->f.r.rs);
    rt = allocate_register((uint32_t*)dst->f.r.rt);
    cmp_reg32_imm32(rt, 0);
    je_rj((rs == EAX ? 0 : 2) + 2 + 2);
    mov_reg32_reg32(EAX, rs); // 0 or 2
    xor_reg32_reg32(EDX, EDX); // 2
    div_reg32(rt); // 2
#endif
}

void gendmult() { gencallinterp((uint32_t)DMULT, 0); }

void gendmultu()
{
#ifdef INTERPRET_DMULTU
    gencallinterp((uint32_t)DMULTU, 0);
#else
    free_all_registers();
    simplify_access();

    mov_eax_memoffs32((uint32_t*)dst->f.r.rs);
    mul_m32((uint32_t*)dst->f.r.rt); // EDX:EAX = temp1
    mov_memoffs32_eax((uint32_t*)(&lo));

    mov_reg32_reg32(EBX, EDX); // EBX = temp1>>32
    mov_eax_memoffs32((uint32_t*)dst->f.r.rs);
    mul_m32((uint32_t*)(dst->f.r.rt) + 1);
    add_reg32_reg32(EBX, EAX);
    adc_reg32_imm32(EDX, 0);
    mov_reg32_reg32(ECX, EDX); // ECX:EBX = temp2

    mov_eax_memoffs32((uint32_t*)(dst->f.r.rs) + 1);
    mul_m32((uint32_t*)dst->f.r.rt); // EDX:EAX = temp3

    add_reg32_reg32(EBX, EAX);
    adc_reg32_imm32(ECX, 0); // ECX:EBX = result2
    mov_m32_reg32((uint32_t*)(&lo) + 1, EBX);

    mov_reg32_reg32(ESI, EDX); // ESI = temp3>>32
    mov_eax_memoffs32((uint32_t*)(dst->f.r.rs) + 1);
    mul_m32((uint32_t*)(dst->f.r.rt) + 1);
    add_reg32_reg32(EAX, ESI);
    adc_reg32_imm32(EDX, 0); // EDX:EAX = temp4

    add_reg32_reg32(EAX, ECX);
    adc_reg32_imm32(EDX, 0); // EDX:EAX = result3
    mov_memoffs32_eax((uint32_t*)(&hi));
    mov_m32_reg32((uint32_t*)(&hi) + 1, EDX);
#endif
}

void genddiv() { gencallinterp((uint32_t)DDIV, 0); }

void genddivu() { gencallinterp((uint32_t)DDIVU, 0); }

void genadd()
{
#ifdef INTERPRET_ADD
    gencallinterp((uint32_t)ADD, 0);
#else
    int32_t rs = allocate_register((uint32_t*)dst->f.r.rs);
    int32_t rt = allocate_register((uint32_t*)dst->f.r.rt);
    int32_t rd = allocate_register_w((uint32_t*)dst->f.r.rd);

    if (rt != rd && rs != rd)
    {
        mov_reg32_reg32(rd, rs);
        add_reg32_reg32(rd, rt);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rs);
        add_reg32_reg32(temp, rt);
        mov_reg32_reg32(rd, temp);
    }
#endif
}

void genaddu()
{
#ifdef INTERPRET_ADDU
    gencallinterp((uint32_t)ADDU, 0);
#else
    int32_t rs = allocate_register((uint32_t*)dst->f.r.rs);
    int32_t rt = allocate_register((uint32_t*)dst->f.r.rt);
    int32_t rd = allocate_register_w((uint32_t*)dst->f.r.rd);

    if (rt != rd && rs != rd)
    {
        mov_reg32_reg32(rd, rs);
        add_reg32_reg32(rd, rt);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rs);
        add_reg32_reg32(temp, rt);
        mov_reg32_reg32(rd, temp);
    }
#endif
}

void gensub()
{
#ifdef INTERPRET_SUB
    gencallinterp((uint32_t)SUB, 0);
#else
    int32_t rs = allocate_register((uint32_t*)dst->f.r.rs);
    int32_t rt = allocate_register((uint32_t*)dst->f.r.rt);
    int32_t rd = allocate_register_w((uint32_t*)dst->f.r.rd);

    if (rt != rd && rs != rd)
    {
        mov_reg32_reg32(rd, rs);
        sub_reg32_reg32(rd, rt);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rs);
        sub_reg32_reg32(temp, rt);
        mov_reg32_reg32(rd, temp);
    }
#endif
}

void gensubu()
{
#ifdef INTERPRET_SUBU
    gencallinterp((uint32_t)SUBU, 0);
#else
    int32_t rs = allocate_register((uint32_t*)dst->f.r.rs);
    int32_t rt = allocate_register((uint32_t*)dst->f.r.rt);
    int32_t rd = allocate_register_w((uint32_t*)dst->f.r.rd);

    if (rt != rd && rs != rd)
    {
        mov_reg32_reg32(rd, rs);
        sub_reg32_reg32(rd, rt);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rs);
        sub_reg32_reg32(temp, rt);
        mov_reg32_reg32(rd, temp);
    }
#endif
}

void genand()
{
#ifdef INTERPRET_AND
    gencallinterp((uint32_t)AND, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    if (rt1 != rd1 && rs1 != rd1)
    {
        mov_reg32_reg32(rd1, rs1);
        mov_reg32_reg32(rd2, rs2);
        and_reg32_reg32(rd1, rt1);
        and_reg32_reg32(rd2, rt2);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rs1);
        and_reg32_reg32(temp, rt1);
        mov_reg32_reg32(rd1, temp);
        mov_reg32_reg32(temp, rs2);
        and_reg32_reg32(temp, rt2);
        mov_reg32_reg32(rd2, temp);
    }
#endif
}

void genor()
{
#ifdef INTERPRET_OR
    gencallinterp((uint32_t)OR, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    if (rt1 != rd1 && rs1 != rd1)
    {
        mov_reg32_reg32(rd1, rs1);
        mov_reg32_reg32(rd2, rs2);
        or_reg32_reg32(rd1, rt1);
        or_reg32_reg32(rd2, rt2);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rs1);
        or_reg32_reg32(temp, rt1);
        mov_reg32_reg32(rd1, temp);
        mov_reg32_reg32(temp, rs2);
        or_reg32_reg32(temp, rt2);
        mov_reg32_reg32(rd2, temp);
    }
#endif
}

void genxor()
{
#ifdef INTERPRET_XOR
    gencallinterp((uint32_t)XOR, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    if (rt1 != rd1 && rs1 != rd1)
    {
        mov_reg32_reg32(rd1, rs1);
        mov_reg32_reg32(rd2, rs2);
        xor_reg32_reg32(rd1, rt1);
        xor_reg32_reg32(rd2, rt2);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rs1);
        xor_reg32_reg32(temp, rt1);
        mov_reg32_reg32(rd1, temp);
        mov_reg32_reg32(temp, rs2);
        xor_reg32_reg32(temp, rt2);
        mov_reg32_reg32(rd2, temp);
    }
#endif
}

void gennor()
{
#ifdef INTERPRET_NOR
    gencallinterp((uint32_t)NOR, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    if (rt1 != rd1 && rs1 != rd1)
    {
        mov_reg32_reg32(rd1, rs1);
        mov_reg32_reg32(rd2, rs2);
        or_reg32_reg32(rd1, rt1);
        or_reg32_reg32(rd2, rt2);
        not_reg32(rd1);
        not_reg32(rd2);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rs1);
        or_reg32_reg32(temp, rt1);
        mov_reg32_reg32(rd1, temp);
        mov_reg32_reg32(temp, rs2);
        or_reg32_reg32(temp, rt2);
        mov_reg32_reg32(rd2, temp);
        not_reg32(rd1);
        not_reg32(rd2);
    }
#endif
}

void genslt()
{
#ifdef INTERPRET_SLT
    gencallinterp((uint32_t)SLT, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd = allocate_register_w((uint32_t*)dst->f.r.rd);

    cmp_reg32_reg32(rs2, rt2);
    jl_rj(13);
    jne_rj(4); // 2
    cmp_reg32_reg32(rs1, rt1); // 2
    jl_rj(7); // 2
    mov_reg32_imm32(rd, 0); // 5
    jmp_imm_short(5); // 2
    mov_reg32_imm32(rd, 1); // 5
#endif
}

void gensltu()
{
#ifdef INTERPRET_SLTU
    gencallinterp((uint32_t)SLTU, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd = allocate_register_w((uint32_t*)dst->f.r.rd);

    cmp_reg32_reg32(rs2, rt2);
    jb_rj(13);
    jne_rj(4); // 2
    cmp_reg32_reg32(rs1, rt1); // 2
    jb_rj(7); // 2
    mov_reg32_imm32(rd, 0); // 5
    jmp_imm_short(5); // 2
    mov_reg32_imm32(rd, 1); // 5
#endif
}

void gendadd()
{
#ifdef INTERPRET_DADD
    gencallinterp((uint32_t)DADD, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    if (rt1 != rd1 && rs1 != rd1)
    {
        mov_reg32_reg32(rd1, rs1);
        mov_reg32_reg32(rd2, rs2);
        add_reg32_reg32(rd1, rt1);
        adc_reg32_reg32(rd2, rt2);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rs1);
        add_reg32_reg32(temp, rt1);
        mov_reg32_reg32(rd1, temp);
        mov_reg32_reg32(temp, rs2);
        adc_reg32_reg32(temp, rt2);
        mov_reg32_reg32(rd2, temp);
    }
#endif
}

void gendaddu()
{
#ifdef INTERPRET_DADDU
    gencallinterp((uint32_t)DADDU, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    if (rt1 != rd1 && rs1 != rd1)
    {
        mov_reg32_reg32(rd1, rs1);
        mov_reg32_reg32(rd2, rs2);
        add_reg32_reg32(rd1, rt1);
        adc_reg32_reg32(rd2, rt2);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rs1);
        add_reg32_reg32(temp, rt1);
        mov_reg32_reg32(rd1, temp);
        mov_reg32_reg32(temp, rs2);
        adc_reg32_reg32(temp, rt2);
        mov_reg32_reg32(rd2, temp);
    }
#endif
}

void gendsub()
{
#ifdef INTERPRET_DSUB
    gencallinterp((uint32_t)DSUB, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    if (rt1 != rd1 && rs1 != rd1)
    {
        mov_reg32_reg32(rd1, rs1);
        mov_reg32_reg32(rd2, rs2);
        sub_reg32_reg32(rd1, rt1);
        sbb_reg32_reg32(rd2, rt2);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rs1);
        sub_reg32_reg32(temp, rt1);
        mov_reg32_reg32(rd1, temp);
        mov_reg32_reg32(temp, rs2);
        sbb_reg32_reg32(temp, rt2);
        mov_reg32_reg32(rd2, temp);
    }
#endif
}

void gendsubu()
{
#ifdef INTERPRET_DSUBU
    gencallinterp((uint32_t)DSUBU, 0);
#else
    int32_t rs1 = allocate_64_register1((uint32_t*)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uint32_t*)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    if (rt1 != rd1 && rs1 != rd1)
    {
        mov_reg32_reg32(rd1, rs1);
        mov_reg32_reg32(rd2, rs2);
        sub_reg32_reg32(rd1, rt1);
        sbb_reg32_reg32(rd2, rt2);
    }
    else
    {
        int32_t temp = lru_register();
        free_register(temp);
        mov_reg32_reg32(temp, rs1);
        sub_reg32_reg32(temp, rt1);
        mov_reg32_reg32(rd1, temp);
        mov_reg32_reg32(temp, rs2);
        sbb_reg32_reg32(temp, rt2);
        mov_reg32_reg32(rd2, temp);
    }
#endif
}

void genteq() { gencallinterp((uint32_t)TEQ, 0); }

void gendsll()
{
#ifdef INTERPRET_DSLL
    gencallinterp((uint32_t)DSLL, 0);
#else
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    mov_reg32_reg32(rd1, rt1);
    mov_reg32_reg32(rd2, rt2);
    shld_reg32_reg32_imm8(rd2, rd1, dst->f.r.sa);
    shl_reg32_imm8(rd1, dst->f.r.sa);
    if (dst->f.r.sa & 0x20)
    {
        mov_reg32_reg32(rd2, rd1);
        xor_reg32_reg32(rd1, rd1);
    }
#endif
}

void gendsrl()
{
#ifdef INTERPRET_DSRL
    gencallinterp((uint32_t)DSRL, 0);
#else
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    mov_reg32_reg32(rd1, rt1);
    mov_reg32_reg32(rd2, rt2);
    shrd_reg32_reg32_imm8(rd1, rd2, dst->f.r.sa);
    shr_reg32_imm8(rd2, dst->f.r.sa);
    if (dst->f.r.sa & 0x20)
    {
        mov_reg32_reg32(rd1, rd2);
        xor_reg32_reg32(rd2, rd2);
    }
#endif
}

void gendsra()
{
#ifdef INTERPRET_DSRA
    gencallinterp((uint32_t)DSRA, 0);
#else
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    mov_reg32_reg32(rd1, rt1);
    mov_reg32_reg32(rd2, rt2);
    shrd_reg32_reg32_imm8(rd1, rd2, dst->f.r.sa);
    sar_reg32_imm8(rd2, dst->f.r.sa);
    if (dst->f.r.sa & 0x20)
    {
        mov_reg32_reg32(rd1, rd2);
        sar_reg32_imm8(rd2, 31);
    }
#endif
}

void gendsll32()
{
#ifdef INTERPRET_DSLL32
    gencallinterp((uint32_t)DSLL32, 0);
#else
    int32_t rt1 = allocate_64_register1((uint32_t*)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    mov_reg32_reg32(rd2, rt1);
    shl_reg32_imm8(rd2, dst->f.r.sa);
    xor_reg32_reg32(rd1, rd1);
#endif
}

void gendsrl32()
{
#ifdef INTERPRET_DSRL32
    gencallinterp((uint32_t)DSRL32, 0);
#else
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uint32_t*)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uint32_t*)dst->f.r.rd);

    mov_reg32_reg32(rd1, rt2);
    shr_reg32_imm8(rd1, dst->f.r.sa);
    xor_reg32_reg32(rd2, rd2);
#endif
}

void gendsra32()
{
#ifdef INTERPRET_DSRA32
    gencallinterp((uint32_t)DSRA32, 0);
#else
    int32_t rt2 = allocate_64_register2((uint32_t*)dst->f.r.rt);
    int32_t rd = allocate_register_w((uint32_t*)dst->f.r.rd);

    mov_reg32_reg32(rd, rt2);
    sar_reg32_imm8(rd, dst->f.r.sa);
#endif
}
