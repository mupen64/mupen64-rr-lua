/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Core.hpp>
#include <Memory/Memory.hpp>
#include <R4300/Exception.hpp>
#include <R4300/Interrupt.hpp>
#include <R4300/Macros.hpp>
#include <R4300/Ops.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomp.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/arm64/Assemble.hpp>
#include <R4300/arm64/RegCache.hpp>

extern uintptr_t g_dyna_target;

[[noreturn]] static void unimplemented_codegen(const char *what)
{
    g_core->log_error(
        std::format("[Dynarec] FATAL: {} has no AArch64 implementation. This backend emits every opcode as an "
                    "interpreter call, so nothing should reach it.",
            what));
    abort();
}

void gen_dispatch_pending()
{
    emit_mov_reg_imm64(A64_IP0, (uintptr_t)&g_dyna_target);
    emit_ldr_x(A64_IP1, A64_IP0);

    const int32_t branch = emit_cbz_x(A64_IP1, 0);
    emit_str_x(A64_ZR, A64_IP0);
    emit_br(A64_IP1);
    patch_cbz_x(branch, (code_length - branch) / 4);
}

precomp_instr fake_instr;

void genupdate_count(uint32_t addr)
{
    emit_mov_reg_imm64(A64_T0, addr);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&last_addr);
    emit_ldr_w_off(A64_T2, A64_T1, 0);
    emit_sub(A64_T0, A64_T0, A64_T2, false);
    emit_lsr_imm_w(A64_T0, A64_T0, 1);

    emit_mov_reg_imm64(A64_T1, (uintptr_t)&core_Count);
    emit_ldr_w_off(A64_T2, A64_T1, 0);
    emit_add(A64_T2, A64_T2, A64_T0, false);
    emit_str_w_off(A64_T2, A64_T1, 0);
}

void gencheck_interrupt_out(uint32_t addr)
{
    emit_mov_reg_imm64(A64_T0, (uintptr_t)&next_interrupt);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Count);
    emit_ldr_w_off(A64_T2, A64_T0, 0);
    emit_cmp_w(A64_T1, A64_T2);

    const int32_t skip = emit_b_cond(A64_COND_HI, 0);

    mov_m32_imm32((void *)(&fake_instr.addr), addr);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(&fake_instr));
    emit_mov_reg_imm64(A64_IP0, (uintptr_t)gen_interrupt);
    emit_blr(A64_IP0);

    gen_dispatch_pending();

    patch_branch(skip, (code_length - skip) / 4);
}

void gencheck_interrupt_reg()
{
    emit_mov_reg_imm64(A64_T0, (uintptr_t)&next_interrupt);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Count);
    emit_ldr_w_off(A64_T3, A64_T0, 0);
    emit_cmp_w(A64_T1, A64_T3);

    const int32_t skip = emit_b_cond(A64_COND_HI, 0);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&fake_instr.addr);
    emit_str_w_off(A64_T2, A64_T0, 0);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(&fake_instr));
    emit_mov_reg_imm64(A64_IP0, (uintptr_t)gen_interrupt);
    emit_blr(A64_IP0);

    gen_dispatch_pending();

    patch_branch(skip, (code_length - skip) / 4);
}

void gendelayslot()
{
    mov_m32_imm32((void *)(&delay_slot), 1);
    recompile_opcode();

    free_all_registers();
    genupdate_count(dst->addr + 4);

    mov_m32_imm32((void *)(&delay_slot), 0);
}

static int32_t gen_branch_taken_is_zero()
{
    emit_mov_reg_imm64(A64_T0, (uintptr_t)&branch_taken);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    return emit_cbz_w(A64_T1, 0);
}

static void gen_store_branch_taken(uint32_t cond)
{
    emit_cset(A64_T2, cond);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&branch_taken);
    emit_str_w_off(A64_T2, A64_T1, 0);
}

void gen_test_rs_rt(uint32_t cond)
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
    const int32_t rt = allocate_register((uintptr_t)dst->f.i.rt);

    emit_cmp_x(rs, rt);
    gen_store_branch_taken(cond);
}

void gen_test_rs_zero(uint32_t cond)
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);

    emit_cmp_x_zero(rs);
    gen_store_branch_taken(cond);
}

void gentest()
{
    const uint32_t target = dst->addr + (dst - 1)->f.i.immediate * 4;
    const int32_t not_taken = gen_branch_taken_is_zero();

    mov_m32_imm32((void *)(&last_addr), target);
    gencheck_interrupt_out(target);
    jmp(target);

    patch_branch(not_taken, (code_length - not_taken) / 4);
    mov_m32_imm32((void *)(&last_addr), dst->addr + 4);
    gencheck_interrupt_out(dst->addr + 4);
    jmp(dst->addr + 4);
}

void gentest_out()
{
    const uint32_t target = dst->addr + (dst - 1)->f.i.immediate * 4;
    const int32_t not_taken = gen_branch_taken_is_zero();

    mov_m32_imm32((void *)(&last_addr), target);
    gencheck_interrupt_out(target);
    mov_m32_imm32((void *)(&jump_to_address), target);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    emit_mov_reg_imm64(A64_IP0, (uintptr_t)jump_to_func);
    emit_blr(A64_IP0);
    gen_dispatch_pending();

    patch_branch(not_taken, (code_length - not_taken) / 4);
    mov_m32_imm32((void *)(&last_addr), dst->addr + 4);
    gencheck_interrupt_out(dst->addr + 4);
    jmp(dst->addr + 4);
}

void gentestl()
{
    gentestl_impl(true);
}

void gentestl_out()
{
    gentestl_out_impl(true);
}

void gentest_idle()
{
    const int32_t not_taken = gen_branch_taken_is_zero();

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&next_interrupt);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Count);
    emit_ldr_w_off(A64_T2, A64_T0, 0);
    emit_sub(A64_T1, A64_T1, A64_T2, false);

    emit_mov_reg_imm64(A64_T3, 3);
    emit_cmp_w(A64_T1, A64_T3);
    const int32_t too_close = emit_b_cond(A64_COND_LS, 0);

    emit_mov_reg_imm64(A64_T3, 0xFFFFFFFCu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_add(A64_T2, A64_T2, A64_T1, false);
    emit_str_w_off(A64_T2, A64_T0, 0);

    patch_branch(too_close, (code_length - too_close) / 4);
    patch_branch(not_taken, (code_length - not_taken) / 4);
}

void gentestl_impl(bool count_on_fallthrough)
{
    const int32_t not_taken = gen_branch_taken_is_zero();

    gendelayslot();
    const uint32_t target = dst->addr + (dst - 1)->f.i.immediate * 4;
    mov_m32_imm32((void *)(&last_addr), target);
    gencheck_interrupt_out(target);
    jmp(target);

    patch_branch(not_taken, (code_length - not_taken) / 4);

    if (count_on_fallthrough) genupdate_count(dst->addr + 4);
    mov_m32_imm32((void *)(&last_addr), dst->addr + 4);
    gencheck_interrupt_out(dst->addr + 4);
    jmp(dst->addr + 4);
}

void gentestl_out_impl(bool count_on_fallthrough)
{
    const int32_t not_taken = gen_branch_taken_is_zero();

    gendelayslot();
    const uint32_t target = dst->addr + (dst - 1)->f.i.immediate * 4;
    mov_m32_imm32((void *)(&last_addr), target);
    gencheck_interrupt_out(target);
    mov_m32_imm32((void *)(&jump_to_address), target);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    emit_mov_reg_imm64(A64_IP0, (uintptr_t)jump_to_func);
    emit_blr(A64_IP0);
    gen_dispatch_pending();

    patch_branch(not_taken, (code_length - not_taken) / 4);

    if (count_on_fallthrough) genupdate_count(dst->addr + 4);
    mov_m32_imm32((void *)(&last_addr), dst->addr + 4);
    gencheck_interrupt_out(dst->addr + 4);
    jmp(dst->addr + 4);
}

void gencallinterp(uintptr_t addr, int32_t jump)
{
    free_all_registers();
    simplify_access();

    if (jump) mov_m32_imm32((void *)(&dyna_interp), 1);

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst));

    (void)addr;
    emit_mov_reg_imm64(A64_IP0, (uintptr_t)(&dst->ops));
    emit_ldr_x(A64_IP0, A64_IP0);
    emit_blr(A64_IP0);

    gen_dispatch_pending();

    if (jump)
    {
        mov_m32_imm32((void *)(&dyna_interp), 0);
        emit_mov_reg_imm64(A64_IP0, (uintptr_t)dyna_jump);
        emit_blr(A64_IP0);
        gen_dispatch_pending();
    }
}

void gennotcompiled()
{
    free_all_registers();
    simplify_access();

    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst));
    emit_mov_reg_imm64(A64_IP0, (uintptr_t)NOTCOMPILED);
    emit_blr(A64_IP0);

    gen_dispatch_pending();
}

void genlink_subblock()
{
    free_all_registers();
    jmp(dst->addr + 4);
}

void genni()
{
    free_all_registers();
}

void gennop()
{
}

void genreserved()
{
    free_all_registers();
}

void gencache()
{
    free_all_registers();
}

void gencheck_cop1_unusable()
{
}

void genfin_block()
{
    free_all_registers();

    gencallinterp((uintptr_t)FIN_BLOCK, 0);
}

void genj()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)J, 1);
        return;
    }

    gendelayslot();

    const uint32_t naddr = ((dst - 1)->f.j.inst_index << 2) | (dst->addr & 0xF0000000);

    mov_m32_imm32((void *)(&last_addr), naddr);
    gencheck_interrupt_out(naddr);
    jmp(naddr);
}

void genj_out()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)J_OUT, 1);
        return;
    }

    gendelayslot();

    const uint32_t naddr = ((dst - 1)->f.j.inst_index << 2) | (dst->addr & 0xF0000000);

    mov_m32_imm32((void *)(&last_addr), naddr);
    gencheck_interrupt_out(naddr);
    mov_m32_imm32((void *)(&jump_to_address), naddr);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    emit_mov_reg_imm64(A64_IP0, (uintptr_t)jump_to_func);
    emit_blr(A64_IP0);
    gen_dispatch_pending();
}

void genj_idle()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)J_IDLE, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&next_interrupt);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Count);
    emit_ldr_w_off(A64_T2, A64_T0, 0);
    emit_sub(A64_T1, A64_T1, A64_T2, false);

    emit_mov_reg_imm64(A64_T3, 3);
    emit_cmp_w(A64_T1, A64_T3);
    const int32_t too_close = emit_b_cond(A64_COND_LS, 0);

    emit_mov_reg_imm64(A64_T3, 0xFFFFFFFCu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_add(A64_T2, A64_T2, A64_T1, false);
    emit_str_w_off(A64_T2, A64_T0, 0);

    patch_branch(too_close, (code_length - too_close) / 4);

    genj();
}

void genjal()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)JAL, 1);
        return;
    }

    gendelayslot();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 4));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    const uint32_t naddr = ((dst - 1)->f.j.inst_index << 2) | (dst->addr & 0xF0000000);

    mov_m32_imm32((void *)(&last_addr), naddr);
    gencheck_interrupt_out(naddr);
    jmp(naddr);
}

void genjal_out()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)JAL_OUT, 1);
        return;
    }

    gendelayslot();

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)(int32_t)(dst->addr + 4));
    emit_str_x_off(A64_T0, A64_RB, reg_offset(&reg[31]));

    const uint32_t naddr = ((dst - 1)->f.j.inst_index << 2) | (dst->addr & 0xF0000000);

    mov_m32_imm32((void *)(&last_addr), naddr);
    gencheck_interrupt_out(naddr);
    mov_m32_imm32((void *)(&jump_to_address), naddr);
    mov_m64_imm64((void *)(&PC), (uintptr_t)(dst + 1));
    emit_mov_reg_imm64(A64_IP0, (uintptr_t)jump_to_func);
    emit_blr(A64_IP0);
    gen_dispatch_pending();
}

void genjal_idle()
{
    free_all_registers();

    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)JAL_IDLE, 1);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&next_interrupt);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Count);
    emit_ldr_w_off(A64_T2, A64_T0, 0);
    emit_sub(A64_T1, A64_T1, A64_T2, false);

    emit_mov_reg_imm64(A64_T3, 3);
    emit_cmp_w(A64_T1, A64_T3);
    const int32_t too_close = emit_b_cond(A64_COND_LS, 0);

    emit_mov_reg_imm64(A64_T3, 0xFFFFFFFCu);
    emit_and(A64_T1, A64_T1, A64_T3, false);
    emit_add(A64_T2, A64_T2, A64_T1, false);
    emit_str_w_off(A64_T2, A64_T0, 0);

    patch_branch(too_close, (code_length - too_close) / 4);

    genjal();
}

void genbeq()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BEQ, 1);
        return;
    }

    gen_test_rs_rt(A64_COND_EQ);
    gendelayslot();
    gentest();
}

void genbeq_out()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BEQ_OUT, 1);
        return;
    }

    gen_test_rs_rt(A64_COND_EQ);
    gendelayslot();
    gentest_out();
}

void genbeq_idle()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BEQ_IDLE, 1);
        return;
    }

    gen_test_rs_rt(A64_COND_EQ);
    gentest_idle();

    genbeq();
}

void genbne()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BNE, 1);
        return;
    }

    gen_test_rs_rt(A64_COND_NE);
    gendelayslot();
    gentest();
}

void genbne_out()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BNE_OUT, 1);
        return;
    }

    gen_test_rs_rt(A64_COND_NE);
    gendelayslot();
    gentest_out();
}

void genbne_idle()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BNE_IDLE, 1);
        return;
    }

    gen_test_rs_rt(A64_COND_NE);
    gentest_idle();

    genbne();
}

void genblez()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLEZ, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LE);
    gendelayslot();
    gentest();
}

void genblez_out()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLEZ_OUT, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LE);
    gendelayslot();
    gentest_out();
}

void genblez_idle()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLEZ_IDLE, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LE);
    gentest_idle();

    genblez();
}

void genbgtz()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGTZ, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GT);
    gendelayslot();
    gentest();
}

void genbgtz_out()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGTZ_OUT, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GT);
    gendelayslot();
    gentest_out();
}

void genbgtz_idle()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGTZ_IDLE, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GT);
    gentest_idle();

    genbgtz();
}

void genaddi()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(rt, rs, A64_T0, false);
    emit_sxtw(rt, rt);
}

void genaddiu()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(rt, rs, A64_T0, false);
    emit_sxtw(rt, rt);
}

void genslti()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_set_lt(rt, rs, A64_T0, true);
}

void gensltiu()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_set_lt(rt, rs, A64_T0, false);
}

void genandi()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    emit_mov_reg_imm64(A64_T0, (uint64_t)(uint16_t)dst->f.i.immediate);
    emit_and(rt, rs, A64_T0, true);
}

void genori()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    emit_mov_reg_imm64(A64_T0, (uint64_t)(uint16_t)dst->f.i.immediate);
    emit_orr(rt, rs, A64_T0, true);
}

void genxori()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    emit_mov_reg_imm64(A64_T0, (uint64_t)(uint16_t)dst->f.i.immediate);
    emit_eor(rt, rs, A64_T0, true);
}

void genlui()
{
    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    emit_mov_reg_imm64(rt, (uint64_t)(int64_t)((int32_t)dst->f.i.immediate << 16));
}

void genbeql()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BEQL, 1);
        return;
    }

    gen_test_rs_rt(A64_COND_EQ);
    free_all_registers();
    gentestl_impl(true);
}

void genbeql_out()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BEQL_OUT, 1);
        return;
    }

    gen_test_rs_rt(A64_COND_EQ);
    free_all_registers();
    gentestl_out_impl(true);
}

void genbeql_idle()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BEQL_IDLE, 1);
        return;
    }

    gen_test_rs_rt(A64_COND_EQ);
    gentest_idle();

    genbeql();
}

void genbnel()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BNEL, 1);
        return;
    }

    gen_test_rs_rt(A64_COND_NE);
    free_all_registers();
    gentestl_impl(true);
}

void genbnel_out()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BNEL_OUT, 1);
        return;
    }

    gen_test_rs_rt(A64_COND_NE);
    free_all_registers();
    gentestl_out_impl(true);
}

void genbnel_idle()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BNEL_IDLE, 1);
        return;
    }

    gen_test_rs_rt(A64_COND_NE);
    gentest_idle();

    genbnel();
}

void genblezl()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLEZL, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LE);
    free_all_registers();
    gentestl_impl(true);
}

void genblezl_out()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLEZL_OUT, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LE);
    free_all_registers();
    gentestl_out_impl(true);
}

void genblezl_idle()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BLEZL_IDLE, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_LE);
    gentest_idle();

    genblezl();
}

void genbgtzl()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGTZL, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GT);
    free_all_registers();
    gentestl_impl(true);
}

void genbgtzl_out()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGTZL_OUT, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GT);
    free_all_registers();
    gentestl_out_impl(true);
}

void genbgtzl_idle()
{
    if (((dst->addr & 0xFFF) == 0xFFC && (dst->addr < 0x80000000 || dst->addr >= 0xC0000000)) ||
        !g_core->cfg->is_compiled_jump_enabled)
    {
        gencallinterp((uintptr_t)BGTZL_IDLE, 1);
        return;
    }

    gen_test_rs_zero(A64_COND_GT);
    gentest_idle();

    genbgtzl();
}

void gendaddi()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(rt, rs, A64_T0, true);
}

void gendaddiu()
{
    const int32_t rs = allocate_register((uintptr_t)dst->f.i.rs);
    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);

    emit_mov_reg_imm64(A64_T0, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(rt, rs, A64_T0, true);
}

void genldl()
{
    free_all_registers();

    gencallinterp((uintptr_t)LDL, 0);
}

void genldr()
{
    free_all_registers();

    gencallinterp((uintptr_t)LDR, 0);
}

void genlb()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)LB, 0);
        return;
    }

    emit_reg_base();
    emit_ldr_w_off(A64_T0, A64_RB, reg_offset(dst->f.i.rs));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T2, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T2, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T2, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 3u);
    emit_eor(A64_T2, A64_T2, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_ldrb_w_reg(A64_T0, A64_T1, A64_T2);
    emit_sxtb(A64_T0, A64_T0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)LB, 0);
    emit_ldr_x_off(A64_T0, A64_RB, reg_offset(dst->f.i.rt));
    patch_branch(to_done, (code_length - to_done) / 4);

    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);
    emit_mov_reg(rt, A64_T0, true);
}

void genlh()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)LH, 0);
        return;
    }

    emit_reg_base();
    emit_ldr_w_off(A64_T0, A64_RB, reg_offset(dst->f.i.rs));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T2, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T2, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T2, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 2u);
    emit_eor(A64_T2, A64_T2, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_ldrh_w_reg(A64_T0, A64_T1, A64_T2);
    emit_sxth(A64_T0, A64_T0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)LH, 0);
    emit_ldr_x_off(A64_T0, A64_RB, reg_offset(dst->f.i.rt));
    patch_branch(to_done, (code_length - to_done) / 4);

    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);
    emit_mov_reg(rt, A64_T0, true);
}

void genlwl()
{
    free_all_registers();

    gencallinterp((uintptr_t)LWL, 0);
}

void genlw()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)LW, 0);
        return;
    }

    emit_reg_base();
    emit_ldr_w_off(A64_T0, A64_RB, reg_offset(dst->f.i.rs));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T2, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T2, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T2, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_ldr_w_reg(A64_T0, A64_T1, A64_T2);
    emit_sxtw(A64_T0, A64_T0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)LW, 0);
    emit_ldr_x_off(A64_T0, A64_RB, reg_offset(dst->f.i.rt));
    patch_branch(to_done, (code_length - to_done) / 4);

    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);
    emit_mov_reg(rt, A64_T0, true);
}

void genlbu()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)LBU, 0);
        return;
    }

    emit_reg_base();
    emit_ldr_w_off(A64_T0, A64_RB, reg_offset(dst->f.i.rs));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T2, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T2, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T2, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 3u);
    emit_eor(A64_T2, A64_T2, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_ldrb_w_reg(A64_T0, A64_T1, A64_T2);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)LBU, 0);
    emit_ldr_x_off(A64_T0, A64_RB, reg_offset(dst->f.i.rt));
    patch_branch(to_done, (code_length - to_done) / 4);

    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);
    emit_mov_reg(rt, A64_T0, true);
}

void genlhu()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)LHU, 0);
        return;
    }

    emit_reg_base();
    emit_ldr_w_off(A64_T0, A64_RB, reg_offset(dst->f.i.rs));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T2, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T2, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T2, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 2u);
    emit_eor(A64_T2, A64_T2, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_ldrh_w_reg(A64_T0, A64_T1, A64_T2);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)LHU, 0);
    emit_ldr_x_off(A64_T0, A64_RB, reg_offset(dst->f.i.rt));
    patch_branch(to_done, (code_length - to_done) / 4);

    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);
    emit_mov_reg(rt, A64_T0, true);
}

void genlwr()
{
    free_all_registers();

    gencallinterp((uintptr_t)LWR, 0);
}

void genlwu()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)LWU, 0);
        return;
    }

    emit_reg_base();
    emit_ldr_w_off(A64_T0, A64_RB, reg_offset(dst->f.i.rs));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T3, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_ldr_w_reg(A64_T2, A64_T1, A64_T3);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)LWU, 0);
    emit_ldr_x_off(A64_T2, A64_RB, reg_offset(dst->f.i.rt));
    patch_branch(to_done, (code_length - to_done) / 4);

    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);
    emit_mov_reg(rt, A64_T2, true);
}

static void gen_check_invalidate()
{
    emit_mov_reg_imm64(A64_T1, (uintptr_t)&address);
    emit_str_w_off(A64_T0, A64_T1, 0);

    emit_lsr_imm_w(A64_T3, A64_T0, 12);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)invalid_code);
    emit_ldrb_w_reg(A64_T2, A64_T1, A64_T3);
    emit_cmp_w(A64_T2, A64_ZR);
    const int32_t already = emit_b_cond(A64_COND_NE, 0);

    emit_lsl_imm_x(A64_T3, A64_T3, 3);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)blocks);
    emit_add(A64_T1, A64_T1, A64_T3, true);
    emit_ldr_x(A64_T2, A64_T1);
    emit_cmp_x_zero(A64_T2);
    const int32_t no_block = emit_b_cond(A64_COND_EQ, 0);

    emit_mov_reg_imm64(A64_IP0, (uintptr_t)dyna_mem_invalidate);
    emit_blr(A64_IP0);

    patch_branch(already, (code_length - already) / 4);
    patch_branch(no_block, (code_length - no_block) / 4);
}

void gensb()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)SB, 0);
        return;
    }

    emit_reg_base();
    emit_ldr_w_off(A64_T2, A64_RB, reg_offset(dst->f.i.rt));
    emit_ldr_w_off(A64_T0, A64_RB, reg_offset(dst->f.i.rs));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T3, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 3u);
    emit_eor(A64_T3, A64_T3, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_strb_w_reg(A64_T2, A64_T1, A64_T3);

    gen_check_invalidate();
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)SB, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gensh()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)SH, 0);
        return;
    }

    emit_reg_base();
    emit_ldr_w_off(A64_T2, A64_RB, reg_offset(dst->f.i.rt));
    emit_ldr_w_off(A64_T0, A64_RB, reg_offset(dst->f.i.rs));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T3, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 2u);
    emit_eor(A64_T3, A64_T3, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_strh_w_reg(A64_T2, A64_T1, A64_T3);

    gen_check_invalidate();
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)SH, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genswl()
{
    free_all_registers();

    gencallinterp((uintptr_t)SWL, 0);
}

void gensw()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)SW, 0);
        return;
    }

    emit_reg_base();
    emit_ldr_w_off(A64_T2, A64_RB, reg_offset(dst->f.i.rt));
    emit_ldr_w_off(A64_T0, A64_RB, reg_offset(dst->f.i.rs));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T3, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_str_w_reg(A64_T2, A64_T1, A64_T3);

    gen_check_invalidate();
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)SW, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void gensdl()
{
    free_all_registers();

    gencallinterp((uintptr_t)SDL, 0);
}

void gensdr()
{
    free_all_registers();

    gencallinterp((uintptr_t)SDR, 0);
}

void genswr()
{
    free_all_registers();

    gencallinterp((uintptr_t)SWR, 0);
}

void genlwc1()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)LWC1, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)LWC1, 0);
    const int32_t from_slow = emit_b(0);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_reg_base();
    emit_ldr_w_off(A64_T0, A64_RB, (uint32_t)(dst->f.lf.base * sizeof(reg[0])));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.lf.offset);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T3, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_ldr_w_reg(A64_T2, A64_T1, A64_T3);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.lf.ft * sizeof(reg_cop1_simple[0])));
    emit_str_w_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)LWC1, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
    patch_branch(from_slow, (code_length - from_slow) / 4);
}

void genldc1()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)LDC1, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)LDC1, 0);
    const int32_t from_slow = emit_b(0);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_reg_base();
    emit_ldr_w_off(A64_T0, A64_RB, (uint32_t)(dst->f.lf.base * sizeof(reg[0])));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.lf.offset);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T3, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_ldr_w_reg(A64_T2, A64_T1, A64_T3);
    emit_lsl_imm_x(A64_T2, A64_T2, 32);
    emit_mov_reg_imm64(A64_IP1, 4);
    emit_add(A64_T3, A64_T3, A64_IP1, false);
    emit_ldr_w_reg(A64_IP1, A64_T1, A64_T3);
    emit_orr(A64_T2, A64_T2, A64_IP1, true);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.lf.ft * sizeof(reg_cop1_double[0])));
    emit_str_x_off(A64_T2, A64_T1, 0);
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)LDC1, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
    patch_branch(from_slow, (code_length - from_slow) / 4);
}

void genld()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)LD, 0);
        return;
    }

    emit_reg_base();
    emit_ldr_w_off(A64_T0, A64_RB, reg_offset(dst->f.i.rs));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T3, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_ldr_w_reg(A64_T2, A64_T1, A64_T3);
    emit_lsl_imm_x(A64_T2, A64_T2, 32);
    emit_mov_reg_imm64(A64_T0, 4);
    emit_add(A64_T3, A64_T3, A64_T0, false);
    emit_ldr_w_reg(A64_T0, A64_T1, A64_T3);
    emit_orr(A64_T2, A64_T2, A64_T0, true);

    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)LD, 0);
    emit_ldr_x_off(A64_T2, A64_RB, reg_offset(dst->f.i.rt));
    patch_branch(to_done, (code_length - to_done) / 4);

    const int32_t rt = allocate_register_w((uintptr_t)dst->f.i.rt);
    emit_mov_reg(rt, A64_T2, true);
}

void genswc1()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)SWC1, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)SWC1, 0);
    const int32_t from_slow = emit_b(0);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_simple);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.lf.ft * sizeof(reg_cop1_simple[0])));
    emit_ldr_w_off(A64_T2, A64_T1, 0);

    emit_reg_base();
    emit_ldr_w_off(A64_T0, A64_RB, (uint32_t)(dst->f.lf.base * sizeof(reg[0])));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.lf.offset);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T3, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_str_w_reg(A64_T2, A64_T1, A64_T3);

    gen_check_invalidate();
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)SWC1, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
    patch_branch(from_slow, (code_length - from_slow) / 4);
}

void gensdc1()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)SDC1, 0);
        return;
    }

    emit_mov_reg_imm64(A64_T0, (uintptr_t)&core_Status);
    emit_ldr_w_off(A64_T1, A64_T0, 0);
    const int32_t cop1_bad = emit_tbz_w(A64_T1, 29, 0);
    const int32_t cop1_ok = emit_b(0);
    patch_branch(cop1_bad, (code_length - cop1_bad) / 4);
    gencallinterp((uintptr_t)SDC1, 0);
    const int32_t from_slow = emit_b(0);
    patch_branch(cop1_ok, (code_length - cop1_ok) / 4);

    emit_mov_reg_imm64(A64_T0, (uintptr_t)reg_cop1_double);
    emit_ldr_x_off(A64_T1, A64_T0, (uint32_t)(dst->f.lf.ft * sizeof(reg_cop1_double[0])));
    emit_ldr_x_off(A64_IP1, A64_T1, 0);

    emit_reg_base();
    emit_ldr_w_off(A64_T0, A64_RB, (uint32_t)(dst->f.lf.base * sizeof(reg[0])));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.lf.offset);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T3, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_lsr_imm_x(A64_T2, A64_IP1, 32);
    emit_str_w_reg(A64_T2, A64_T1, A64_T3);
    emit_mov_reg_imm64(A64_T2, 4);
    emit_add(A64_T3, A64_T3, A64_T2, false);
    emit_str_w_reg(A64_IP1, A64_T1, A64_T3);

    gen_check_invalidate();
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)SDC1, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
    patch_branch(from_slow, (code_length - from_slow) / 4);
}

void gensd()
{
    free_all_registers();

    if (!fast_memory)
    {
        gencallinterp((uintptr_t)SD, 0);
        return;
    }

    emit_reg_base();
    emit_ldr_x_off(A64_IP1, A64_RB, reg_offset(dst->f.i.rt));
    emit_ldr_w_off(A64_T0, A64_RB, reg_offset(dst->f.i.rs));
    emit_mov_reg_imm64(A64_T1, (uint64_t)(int64_t)dst->f.i.immediate);
    emit_add(A64_T0, A64_T0, A64_T1, false);

    emit_mov_reg_imm64(A64_T1, 0xDF800000u);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, 0x80000000u);
    emit_cmp_w(A64_T3, A64_T1);
    const int32_t to_slow = emit_b_cond(A64_COND_NE, 0);

    emit_mov_reg_imm64(A64_T1, 0x7FFFFFu);
    emit_and(A64_T3, A64_T0, A64_T1, false);
    emit_mov_reg_imm64(A64_T1, (uintptr_t)rdram);
    emit_lsr_imm_x(A64_T2, A64_IP1, 32);
    emit_str_w_reg(A64_T2, A64_T1, A64_T3);
    emit_mov_reg_imm64(A64_T2, 4);
    emit_add(A64_T3, A64_T3, A64_T2, false);
    emit_str_w_reg(A64_IP1, A64_T1, A64_T3);

    gen_check_invalidate();
    const int32_t to_done = emit_b(0);

    patch_branch(to_slow, (code_length - to_slow) / 4);
    gencallinterp((uintptr_t)SD, 0);
    patch_branch(to_done, (code_length - to_done) / 4);
}

void genll()
{
    free_all_registers();

    gencallinterp((uintptr_t)LL, 0);
}

void gensc()
{
    free_all_registers();

    gencallinterp((uintptr_t)SC, 0);
}

void gendebug()
{
    unimplemented_codegen("gendebug");
}
