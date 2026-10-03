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

void gensync()
{
    free_all_registers();
}

void gensll()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_lsl_imm_w(rd, rt, dst->f.r.sa);
    emit_sxtw(rd, rd);
}

void gensrl()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_lsr_imm_w(rd, rt, dst->f.r.sa);
    emit_sxtw(rd, rd);
}

void gensra()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_asr_imm_w(rd, rt, dst->f.r.sa);
    emit_sxtw(rd, rd);
}

void gensllv()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_lslv_w(rd, rt, rs);
    emit_sxtw(rd, rd);
}

void gensrlv()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_lsrv_w(rd, rt, rs);
    emit_sxtw(rd, rd);
}

void gensrav()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_asrv_w(rd, rt, rs);
    emit_sxtw(rd, rd);
}

void genjr()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)JR, 1);
        return;
    }

    emit_ldr_w_off(A64_T2, A64_RB, reg_offset(dst->f.i.rs));
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&local_rs);
    emit_str_w_off(A64_T2, A64_T1, 0);

    gendelayslot();

    emit_mov_reg_imm64(A64_T1, (uintptr_t)&local_rs);
    emit_ldr_w_off(A64_T2, A64_T1, 0);

    emit_mov_reg_imm64(A64_T1, (uintptr_t)&last_addr);
    emit_str_w_off(A64_T2, A64_T1, 0);
    gencheck_interrupt_reg();

    emit_mov_reg_imm64(A64_T1, (uintptr_t)&local_rs);
    emit_ldr_w_off(A64_T2, A64_T1, 0);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&jump_to_address);
    emit_str_w_off(A64_T2, A64_T1, 0);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    emit_mov_reg_imm64(A64_IP0, (uintptr_t)jump_to_func);
    emit_blr(A64_IP0);
    gen_dispatch_pending();
}

void genjalr()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)JALR, 1);
        return;
    }

    emit_ldr_w_off(A64_T2, A64_RB, reg_offset(dst->f.i.rs));
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&local_rs);
    emit_str_w_off(A64_T2, A64_T1, 0);

    gendelayslot();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 4));
    emit_str_x_off(A64_T0, A64_RB, reg_offset((dst - 1)->f.r.rd));

    emit_mov_reg_imm64(A64_T1, (uintptr_t)&local_rs);
    emit_ldr_w_off(A64_T2, A64_T1, 0);

    emit_mov_reg_imm64(A64_T1, (uintptr_t)&last_addr);
    emit_str_w_off(A64_T2, A64_T1, 0);
    gencheck_interrupt_reg();

    emit_mov_reg_imm64(A64_T1, (uintptr_t)&local_rs);
    emit_ldr_w_off(A64_T2, A64_T1, 0);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&jump_to_address);
    emit_str_w_off(A64_T2, A64_T1, 0);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    emit_mov_reg_imm64(A64_IP0, (uintptr_t)jump_to_func);
    emit_blr(A64_IP0);
    gen_dispatch_pending();
}

void gensyscall()
{
    free_all_registers();
    simplify_access();

    mov_m32_imm32((void *)&core_Cause, 8 << 2);
    gencallinterp((uintptr_t)exception_general, 0);
}

void genmfhi()
{
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_mov_reg_imm64(A64_T1, (uintptr_t)&hi);
    emit_ldr_x_off(rd, A64_T1, 0);
}

void genmthi()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);

    emit_mov_reg_imm64(A64_T1, (uintptr_t)&hi);
    emit_str_x_off(rs, A64_T1, 0);
}

void genmflo()
{
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_mov_reg_imm64(A64_T1, (uintptr_t)&lo);
    emit_ldr_x_off(rd, A64_T1, 0);
}

void genmtlo()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);

    emit_mov_reg_imm64(A64_T1, (uintptr_t)&lo);
    emit_str_x_off(rs, A64_T1, 0);
}

void gendsllv()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_lslv_x(rd, rt, rs);
}

void gendsrlv()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_lsrv_x(rd, rt, rs);
}

void gendsrav()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_asrv_x(rd, rt, rs);
}

void genmult()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);

    emit_mul_x(A64_T0, rs, rt);

    emit_asr_imm_x(A64_T2, A64_T0, 32);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&hi);
    emit_str_x_off(A64_T2, A64_T1, 0);

    emit_sxtw(A64_T0, A64_T0);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&lo);
    emit_str_x_off(A64_T0, A64_T1, 0);
}

void genmultu()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);

    emit_umull(A64_T0, rs, rt);

    emit_asr_imm_x(A64_T2, A64_T0, 32);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&hi);
    emit_str_x_off(A64_T2, A64_T1, 0);

    emit_sxtw(A64_T0, A64_T0);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&lo);
    emit_str_x_off(A64_T0, A64_T1, 0);
}

void gendiv()
{
    free_all_registers();

    emit_reg_base();
    emit_ldr_w_off(A64_T0, A64_RB, reg_offset(dst->f.r.rs));
    emit_ldr_w_off(A64_T1, A64_RB, reg_offset(dst->f.r.rt));

    const int32_t to_slow = emit_cbz_w(A64_T1, 0);

    emit_sdiv_w(A64_T2, A64_T0, A64_T1);
    emit_msub_w(A64_T3, A64_T2, A64_T1, A64_T0);

    emit_sxtw(A64_T2, A64_T2);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&lo);
    emit_str_x_off(A64_T2, A64_T1, 0);

    emit_sxtw(A64_T3, A64_T3);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&hi);
    emit_str_x_off(A64_T3, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)DIV, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gendivu()
{
    free_all_registers();

    emit_reg_base();
    emit_ldr_w_off(A64_T0, A64_RB, reg_offset(dst->f.r.rs));
    emit_ldr_w_off(A64_T1, A64_RB, reg_offset(dst->f.r.rt));

    const int32_t to_slow = emit_cbz_w(A64_T1, 0);

    emit_udiv_w(A64_T2, A64_T0, A64_T1);
    emit_msub_w(A64_T3, A64_T2, A64_T1, A64_T0);

    emit_sxtw(A64_T2, A64_T2);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&lo);
    emit_str_x_off(A64_T2, A64_T1, 0);

    emit_sxtw(A64_T3, A64_T3);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&hi);
    emit_str_x_off(A64_T3, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)DIVU, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gendmult()
{
    free_all_registers();

    gencallinterp((uintptr_t)DMULT, 0);
}

void gendmultu()
{
    free_all_registers();

    emit_reg_base();
    emit_ldr_x_off(A64_T0, A64_RB, reg_offset(dst->f.r.rs));
    emit_ldr_x_off(A64_T1, A64_RB, reg_offset(dst->f.r.rt));

    emit_umulh(A64_T2, A64_T0, A64_T1);
    emit_mul_x(A64_T0, A64_T0, A64_T1);

    emit_mov_reg_imm64(A64_T1, (uintptr_t)&lo);
    emit_str_x_off(A64_T0, A64_T1, 0);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&hi);
    emit_str_x_off(A64_T2, A64_T1, 0);
}

void genddiv()
{
    free_all_registers();

    gencallinterp((uintptr_t)DDIV, 0);
}

void genddivu()
{
    free_all_registers();

    gencallinterp((uintptr_t)DDIVU, 0);
}

void genadd()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_add(rd, rs, rt, false);
    emit_sxtw(rd, rd);
}

void genaddu()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_add(rd, rs, rt, false);
    emit_sxtw(rd, rd);
}

void gensub()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_sub(rd, rs, rt, false);
    emit_sxtw(rd, rd);
}

void gensubu()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_sub(rd, rs, rt, false);
    emit_sxtw(rd, rd);
}

void genand()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_and(rd, rs, rt, true);
}

void genor()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_orr(rd, rs, rt, true);
}

void genxor()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_eor(rd, rs, rt, true);
}

void gennor()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_orr(rd, rs, rt, true);
    emit_mvn(rd, rd, true);
}

void genslt()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_set_lt(rd, rs, rt, true);
}

void gensltu()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_set_lt(rd, rs, rt, false);
}

void gendadd()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_add(rd, rs, rt, true);
}

void gendaddu()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_add(rd, rs, rt, true);
}

void gendsub()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_sub(rd, rs, rt, true);
}

void gendsubu()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.r.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_sub(rd, rs, rt, true);
}

void genteq()
{
    free_all_registers();

    gencallinterp((uintptr_t)TEQ, 0);
}

void gendsll()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_lsl_imm_x(rd, rt, dst->f.r.sa);
}

void gendsrl()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_lsr_imm_x(rd, rt, dst->f.r.sa);
}

void gendsra()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_asr_imm_x(rd, rt, dst->f.r.sa);
}

void gendsll32()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_lsl_imm_x(rd, rt, (uint32_t)dst->f.r.sa + 32);
}

void gendsrl32()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_lsr_imm_x(rd, rt, (uint32_t)dst->f.r.sa + 32);
}

void gendsra32()
{
    const int32_t rt = allocate_register((uintptr_t)dst->f.r.rt);
    const int32_t rd = allocate_register_w((uintptr_t)dst->f.r.rd);

    emit_asr_imm_x(rd, rt, (uint32_t)dst->f.r.sa + 32);
}
