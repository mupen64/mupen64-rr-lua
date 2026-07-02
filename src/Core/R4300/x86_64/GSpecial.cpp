/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include <Core.hpp>
#include <R4300/Exception.hpp>
#include <R4300/Macros.hpp>
#include <R4300/Ops.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomp.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/x86_64/Assemble.hpp>
#include <R4300/x86_64/RegCache.hpp>


void gensll()
{
#ifdef INTERPRET_SLL
    gencallinterp((uintptr_t)SLL, 0);
#else
    int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    mov_reg32_reg32(rd, rt);
    shl_reg32_imm8(rd, dst->f.r.sa);
#endif
}

void gensrl()
{
#ifdef INTERPRET_SRL
    gencallinterp((uintptr_t)SRL, 0);
#else
    int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    mov_reg32_reg32(rd, rt);
    shr_reg32_imm8(rd, dst->f.r.sa);
#endif
}

void gensra()
{
#ifdef INTERPRET_SRA
    gencallinterp((uintptr_t)SRA, 0);
#else
    int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    mov_reg32_reg32(rd, rt);
    sar_reg32_imm8(rd, dst->f.r.sa);
#endif
}

void gensllv()
{
#ifdef INTERPRET_SLLV
    gencallinterp((uintptr_t)SLLV, 0);
#else
    int32_t rt, rd;
    allocate_register_manually(ECX, (uintptr_t)dst->f.r.rs);

    rt = allocate_register((uintptr_t)dst->f.r.rt);
    rd = allocate_register_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)SRLV, 0);
#else
    int32_t rt, rd;
    allocate_register_manually(ECX, (uintptr_t)dst->f.r.rs);

    rt = allocate_register((uintptr_t)dst->f.r.rt);
    rd = allocate_register_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)SRAV, 0);
#else
    int32_t rt, rd;
    allocate_register_manually(ECX, (uintptr_t)dst->f.r.rs);

    rt = allocate_register((uintptr_t)dst->f.r.rt);
    rd = allocate_register_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)JR, 1);
#else
    static uint32_t precomp_instr_size = sizeof(precomp_instr);
    uintptr_t diff = (uintptr_t)(&dst->local_addr) - (uintptr_t)(dst);
    uintptr_t diff_need = (uintptr_t)(&dst->reg_cache_infos.need_map) - (uintptr_t)(dst);
    uintptr_t diff_wrap = (uintptr_t)(&dst->reg_cache_infos.jump_wrapper) - (uintptr_t)(dst);
    uint32_t temp, temp2;

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)JR, 1);
        return;
    }

    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t *)dst->f.i.rs);
    mov_memoffs32_eax((uint32_t *)&local_rs);

    gendelayslot();

    mov_eax_memoffs32((uint32_t *)&local_rs);
    mov_memoffs32_eax((uint32_t *)&last_addr);

    gencheck_interrupt_reg();

    mov_eax_memoffs32((uint32_t *)&local_rs);
    mov_reg32_reg32(EBX, EAX);
    and_eax_imm32(0xFFFFF000);
    cmp_eax_imm32(dst_block->start & 0xFFFFF000);
    je_near_rj(0);
    temp = code_length;

    mov_m32_reg32(&jump_to_address, EBX);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_reg64_imm64(EAX, (uintptr_t)jump_to_func);
    call_reg64(EAX);

    // Out-of-block target: jump_to_func has set up the deferred redirect. Skip the
    // in-block address computation below — for an out-of-page target it would build
    // a bogus local_addr from (target - dst_block->start) (a huge/negative index)
    // and jump into uncompiled, zeroed code. Mirror genj_out: after the call, leave
    // the sequence entirely.
    jmp_imm(0);
    int32_t jr_skip = code_length - 4;

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;

    mov_reg32_reg32(EAX, EBX);
    sub_eax_imm32(dst_block->start);
    shr_reg32_imm8(EAX, 2);
    mul_m32((uint32_t *)(&precomp_instr_size));

    // x64: dst_block->block and dst_block->code are 64-bit pointers. The old
    // x86 tail folded them into 32-bit immediates / 32-bit memory operands,
    // truncating the high half so the computed jump target landed in a random
    // low (non-executable) address (crash "execute non-executable address").
    // Compute the full 64-bit targets instead.

    // RDX = byte offset into block[] (32-bit mul zeroes the high half of RDX/RAX)
    mov_reg32_reg32(EDX, EAX);

    // EBX = block[index].need_map, loaded via a full 64-bit base
    mov_reg64_imm64(ECX, (uintptr_t)dst_block->block + diff_need);
    add_reg64_reg64(ECX, EDX);
    mov_reg32_preg64(EBX, ECX);
    cmp_reg32_imm32(EBX, 1);
    jne_near_rj(16); // skip the need_map branch (10 + 3 + 3 = 16 bytes)

    // need_map == 1: jump to block[index].jump_wrapper[0]
    mov_reg64_imm64(EAX, (uintptr_t)dst_block->block + diff_wrap); // 10
    add_reg64_reg64(EAX, EDX);                                     // 3
    jmp_reg64(EAX);                                                // 3

    // need_map == 0: jump to block->code + block[index].local_addr
    mov_reg64_imm64(EAX, (uintptr_t)dst_block->block + diff);      // 10
    add_reg64_reg64(EAX, EDX);                                     // 3  -> RAX = &block[index].local_addr
    mov_reg64_preg64(EAX, EAX);                                    // 3  -> RAX = local_addr
    // Read dst_block->code from memory at RUNTIME, not as a baked imm64. The
    // block's code buffer is reallocated (moved) when it grows, so an absolute
    // base captured at generation time goes stale: the JR would jump to
    // OLD_code + local_addr, landing in the abandoned pre-grow buffer (zeros).
    // The ADDRESS of the code field is stable, so load through it live.
    mov_reg64_imm64(ECX, (uintptr_t)&dst_block->code);           // 10 -> RCX = &block->code
    mov_reg64_preg64(ECX, ECX);                                  // 3  -> RCX = block->code (live)
    add_reg64_reg64(EAX, ECX);                                   // 3  -> RAX = code + local_addr
    jmp_reg64(EAX);                                              // 3

    rj_patch_near(jr_skip); // out-of-block path lands here (past the in-block jump)
#endif
}

void genjalr()
{
#ifdef INTERPRET_JALR
    gencallinterp((uintptr_t)JALR, 0);
#else
    static uint32_t precomp_instr_size = sizeof(precomp_instr);
    uintptr_t diff = (uintptr_t)(&dst->local_addr) - (uintptr_t)(dst);
    uintptr_t diff_need = (uintptr_t)(&dst->reg_cache_infos.need_map) - (uintptr_t)(dst);
    uintptr_t diff_wrap = (uintptr_t)(&dst->reg_cache_infos.jump_wrapper) - (uintptr_t)(dst);
    uint32_t temp, temp2;

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)JALR, 1);
        return;
    }

    free_all_registers();
    simplify_access();
    mov_eax_memoffs32((uint32_t *)dst->f.r.rs);
    mov_memoffs32_eax((uint32_t *)&local_rs);

    gendelayslot();

    mov_m32_imm32((uint32_t *)(dst - 1)->f.r.rd, dst->addr + 4);
    if ((dst->addr + 4) & 0x80000000)
        mov_m32_imm32(((uint32_t *)(dst - 1)->f.r.rd) + 1, 0xFFFFFFFF);
    else
        mov_m32_imm32(((uint32_t *)(dst - 1)->f.r.rd) + 1, 0);

    mov_eax_memoffs32((uint32_t *)&local_rs);
    mov_memoffs32_eax((uint32_t *)&last_addr);

    gencheck_interrupt_reg();

    mov_eax_memoffs32((uint32_t *)&local_rs);
    mov_reg32_reg32(EBX, EAX);
    and_eax_imm32(0xFFFFF000);
    cmp_eax_imm32(dst_block->start & 0xFFFFF000);
    je_near_rj(0);
    temp = code_length;

    mov_m32_reg32(&jump_to_address, EBX);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    mov_reg64_imm64(EAX, (uintptr_t)jump_to_func);
    call_reg64(EAX);

    // Out-of-block target: jump_to_func has set up the deferred redirect. Skip the
    // in-block address computation below — for an out-of-page target it would build
    // a bogus local_addr from (target - dst_block->start) (a huge/negative index)
    // and jump into uncompiled, zeroed code. Mirror genj_out: after the call, leave
    // the sequence entirely.
    jmp_imm(0);
    int32_t jr_skip = code_length - 4;

    temp2 = code_length;
    code_length = temp - 4;
    put32(temp2 - temp);
    code_length = temp2;

    mov_reg32_reg32(EAX, EBX);
    sub_eax_imm32(dst_block->start);
    shr_reg32_imm8(EAX, 2);
    mul_m32((uint32_t *)(&precomp_instr_size));

    // x64: use full 64-bit bases for block[]/->code. The old 32-bit tail folded
    // dst_block->block/->code into 32-bit adds, truncating the high half so the
    // jump landed at a bogus low (non-executable) address. Mirror genjr.
    mov_reg32_reg32(EDX, EAX); // RDX = byte offset into block[]

    mov_reg64_imm64(ECX, (uintptr_t)dst_block->block + diff_need);
    add_reg64_reg64(ECX, EDX);
    mov_reg32_preg64(EBX, ECX);
    cmp_reg32_imm32(EBX, 1);
    jne_near_rj(16); // skip the need_map branch (10 + 3 + 3 = 16 bytes)

    // need_map == 1: jump to block[index].jump_wrapper[0]
    mov_reg64_imm64(EAX, (uintptr_t)dst_block->block + diff_wrap); // 10
    add_reg64_reg64(EAX, EDX);                                     // 3
    jmp_reg64(EAX);                                                // 3

    // need_map == 0: jump to block->code + block[index].local_addr
    mov_reg64_imm64(EAX, (uintptr_t)dst_block->block + diff);      // 10
    add_reg64_reg64(EAX, EDX);                                     // 3
    mov_reg64_preg64(EAX, EAX);                                    // 3
    mov_reg64_imm64(ECX, (uintptr_t)&dst_block->code);            // 10
    mov_reg64_preg64(ECX, ECX);                                  // 3
    add_reg64_reg64(EAX, ECX);                                   // 3
    jmp_reg64(EAX);                                              // 3

    rj_patch_near(jr_skip); // out-of-block path lands here (past the in-block jump)
#endif
}

void gensyscall()
{
#ifdef INTERPRET_SYSCALL
    gencallinterp((uintptr_t)SYSCALL, 0);
#else
    free_all_registers();
    simplify_access();
    mov_m32_imm32(&core_Cause, 8 << 2);
    gencallinterp((uintptr_t)exception_general, 0);
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
    gencallinterp((uintptr_t)MFHI, 0);
#else
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);
    int32_t hi1 = allocate_64_register1((uintptr_t)&hi);
    int32_t hi2 = allocate_64_register2((uintptr_t)&hi);

    mov_reg32_reg32(rd1, hi1);
    mov_reg32_reg32(rd2, hi2);
#endif
}

void genmthi()
{
#ifdef INTERPRET_MTHI
    gencallinterp((uintptr_t)MTHI, 0);
#else
    int32_t hi1 = allocate_64_register1_w((uintptr_t)&hi);
    int32_t hi2 = allocate_64_register2_w((uintptr_t)&hi);
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.r.rs);

    mov_reg32_reg32(hi1, rs1);
    mov_reg32_reg32(hi2, rs2);
#endif
}

void genmflo()
{
#ifdef INTERPRET_MFLO
    gencallinterp((uintptr_t)MFLO, 0);
#else
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);
    int32_t lo1 = allocate_64_register1((uintptr_t)&lo);
    int32_t lo2 = allocate_64_register2((uintptr_t)&lo);

    mov_reg32_reg32(rd1, lo1);
    mov_reg32_reg32(rd2, lo2);
#endif
}

void genmtlo()
{
#ifdef INTERPRET_MTLO
    gencallinterp((uintptr_t)MTLO, 0);
#else
    int32_t lo1 = allocate_64_register1_w((uintptr_t)&lo);
    int32_t lo2 = allocate_64_register2_w((uintptr_t)&lo);
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.r.rs);

    mov_reg32_reg32(lo1, rs1);
    mov_reg32_reg32(lo2, rs2);
#endif
}

void gendsllv()
{
#ifdef INTERPRET_DSLLV
    gencallinterp((uintptr_t)DSLLV, 0);
#else
    int32_t rt1, rt2, rd1, rd2;
    allocate_register_manually(ECX, (uintptr_t)dst->f.r.rs);

    rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)DSRLV, 0);
#else
    int32_t rt1, rt2, rd1, rd2;
    allocate_register_manually(ECX, (uintptr_t)dst->f.r.rs);

    rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)DSRAV, 0);
#else
    int32_t rt1, rt2, rd1, rd2;
    allocate_register_manually(ECX, (uintptr_t)dst->f.r.rs);

    rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

    if (rd1 != ECX && rd2 != ECX)
    {
        mov_reg32_reg32(rd1, rt1);
        mov_reg32_reg32(rd2, rt2);
        shrd_reg32_reg32_cl(rd1, rd2);
        sar_reg32_cl(rd2);
        test_reg32_imm32(ECX, 0x20);
        je_rj(5);
        mov_reg32_reg32(rd1, rd2); // 2
        sar_reg32_imm8(rd2, 31);   // 3
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
        sar_reg32_imm8(temp2, 31);     // 3

        mov_reg32_reg32(rd1, temp1);
        mov_reg32_reg32(rd2, temp2);
    }
#endif
}

void genmult()
{
#ifdef INTERPRET_MULT
    gencallinterp((uintptr_t)MULT, 0);
#else
    int32_t rs, rt;
    allocate_register_manually_w(EAX, (uintptr_t)&lo, 0);
    allocate_register_manually_w(EDX, (uintptr_t)&hi, 0);
    rs = allocate_register((uintptr_t)dst->f.r.rs);
    rt = allocate_register((uintptr_t)dst->f.r.rt);
    mov_reg32_reg32(EAX, rs);
    imul_reg32(rt);
#endif
}

void genmultu()
{
#ifdef INTERPRET_MULTU
    gencallinterp((uintptr_t)MULTU, 0);
#else
    int32_t rs, rt;
    allocate_register_manually_w(EAX, (uintptr_t)&lo, 0);
    allocate_register_manually_w(EDX, (uintptr_t)&hi, 0);
    rs = allocate_register((uintptr_t)dst->f.r.rs);
    rt = allocate_register((uintptr_t)dst->f.r.rt);
    mov_reg32_reg32(EAX, rs);
    mul_reg32(rt);
#endif
}

void gendiv()
{
#ifdef INTERPRET_DIV
    gencallinterp((uintptr_t)DIV, 0);
#else
    int32_t rs, rt;
    allocate_register_manually_w(EAX, (uintptr_t)&lo, 0);
    allocate_register_manually_w(EDX, (uintptr_t)&hi, 0);
    rs = allocate_register((uintptr_t)dst->f.r.rs);
    rt = allocate_register((uintptr_t)dst->f.r.rt);
    cmp_reg32_imm32(rt, 0);
    je_rj((rs == EAX ? 0 : 2) + 1 + 2);
    mov_reg32_reg32(EAX, rs); // 0 or 2
    cdq();                    // 1
    idiv_reg32(rt);           // 2
#endif
}

void gendivu()
{
#ifdef INTERPRET_DIVU
    gencallinterp((uintptr_t)DIVU, 0);
#else
    int32_t rs, rt;
    allocate_register_manually_w(EAX, (uintptr_t)&lo, 0);
    allocate_register_manually_w(EDX, (uintptr_t)&hi, 0);
    rs = allocate_register((uintptr_t)dst->f.r.rs);
    rt = allocate_register((uintptr_t)dst->f.r.rt);
    cmp_reg32_imm32(rt, 0);
    je_rj((rs == EAX ? 0 : 2) + 2 + 2);
    mov_reg32_reg32(EAX, rs);  // 0 or 2
    xor_reg32_reg32(EDX, EDX); // 2
    div_reg32(rt);             // 2
#endif
}

void gendmult()
{
    gencallinterp((uintptr_t)DMULT, 0);
}

void gendmultu()
{
#ifdef INTERPRET_DMULTU
    gencallinterp((uintptr_t)DMULTU, 0);
#else
    free_all_registers();
    simplify_access();

    mov_eax_memoffs32((uint32_t *)dst->f.r.rs);
    mul_m32((uint32_t *)dst->f.r.rt); // EDX:EAX = temp1
    mov_memoffs32_eax((uint32_t *)(&lo));

    mov_reg32_reg32(EBX, EDX); // EBX = temp1>>32
    mov_eax_memoffs32((uint32_t *)dst->f.r.rs);
    mul_m32((uint32_t *)(dst->f.r.rt) + 1);
    add_reg32_reg32(EBX, EAX);
    adc_reg32_imm32(EDX, 0);
    mov_reg32_reg32(ECX, EDX); // ECX:EBX = temp2

    mov_eax_memoffs32((uint32_t *)(dst->f.r.rs) + 1);
    mul_m32((uint32_t *)dst->f.r.rt); // EDX:EAX = temp3

    add_reg32_reg32(EBX, EAX);
    adc_reg32_imm32(ECX, 0); // ECX:EBX = result2
    mov_m32_reg32((uint32_t *)(&lo) + 1, EBX);

    mov_reg32_reg32(ESI, EDX); // ESI = temp3>>32
    mov_eax_memoffs32((uint32_t *)(dst->f.r.rs) + 1);
    mul_m32((uint32_t *)(dst->f.r.rt) + 1);
    add_reg32_reg32(EAX, ESI);
    adc_reg32_imm32(EDX, 0); // EDX:EAX = temp4

    add_reg32_reg32(EAX, ECX);
    adc_reg32_imm32(EDX, 0); // EDX:EAX = result3
    mov_memoffs32_eax((uint32_t *)(&hi));
    mov_m32_reg32((uint32_t *)(&hi) + 1, EDX);
#endif
}

void genddiv()
{
    gencallinterp((uintptr_t)DDIV, 0);
}

void genddivu()
{
    gencallinterp((uintptr_t)DDIVU, 0);
}

void genadd()
{
#ifdef INTERPRET_ADD
    gencallinterp((uintptr_t)ADD, 0);
#else
    int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)ADDU, 0);
#else
    int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)SUB, 0);
#else
    int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)SUBU, 0);
#else
    int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)AND, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)OR, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)XOR, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)NOR, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)SLT, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    cmp_reg32_reg32(rs2, rt2);
    jl_rj(13);
    jne_rj(4);                 // 2
    cmp_reg32_reg32(rs1, rt1); // 2
    jl_rj(7);                  // 2
    mov_reg32_imm32(rd, 0);    // 5
    jmp_imm_short(5);          // 2
    mov_reg32_imm32(rd, 1);    // 5
#endif
}

void gensltu()
{
#ifdef INTERPRET_SLTU
    gencallinterp((uintptr_t)SLTU, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    cmp_reg32_reg32(rs2, rt2);
    jb_rj(13);
    jne_rj(4);                 // 2
    cmp_reg32_reg32(rs1, rt1); // 2
    jb_rj(7);                  // 2
    mov_reg32_imm32(rd, 0);    // 5
    jmp_imm_short(5);          // 2
    mov_reg32_imm32(rd, 1);    // 5
#endif
}

void gendadd()
{
#ifdef INTERPRET_DADD
    gencallinterp((uintptr_t)DADD, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)DADDU, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)DSUB, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)DSUBU, 0);
#else
    int32_t rs1 = allocate_64_register1((uintptr_t)dst->f.r.rs);
    int32_t rs2 = allocate_64_register2((uintptr_t)dst->f.r.rs);
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

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

void genteq()
{
    gencallinterp((uintptr_t)TEQ, 0);
}

void gendsll()
{
#ifdef INTERPRET_DSLL
    gencallinterp((uintptr_t)DSLL, 0);
#else
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)DSRL, 0);
#else
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)DSRA, 0);
#else
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

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
    gencallinterp((uintptr_t)DSLL32, 0);
#else
    int32_t rt1 = allocate_64_register1((uintptr_t)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

    mov_reg32_reg32(rd2, rt1);
    shl_reg32_imm8(rd2, dst->f.r.sa);
    xor_reg32_reg32(rd1, rd1);
#endif
}

void gendsrl32()
{
#ifdef INTERPRET_DSRL32
    gencallinterp((uintptr_t)DSRL32, 0);
#else
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd1 = allocate_64_register1_w((uintptr_t)dst->f.r.rd);
    int32_t rd2 = allocate_64_register2_w((uintptr_t)dst->f.r.rd);

    mov_reg32_reg32(rd1, rt2);
    shr_reg32_imm8(rd1, dst->f.r.sa);
    xor_reg32_reg32(rd2, rd2);
#endif
}

void gendsra32()
{
#ifdef INTERPRET_DSRA32
    gencallinterp((uintptr_t)DSRA32, 0);
#else
    int32_t rt2 = allocate_64_register2((uintptr_t)dst->f.r.rt);
    int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    mov_reg32_reg32(rd, rt2);
    sar_reg32_imm8(rd, dst->f.r.sa);
#endif
}
