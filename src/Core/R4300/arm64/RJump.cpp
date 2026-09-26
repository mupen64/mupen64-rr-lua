/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <Core.hpp>
#include <R4300/R4300.hpp>
#include <R4300/Recomp.hpp>
#include <R4300/Recomph.hpp>
#include <R4300/arm64/Assemble.hpp>
#include <R4300/arm64/RegCache.hpp>
#include <Alloc.hpp>

uintptr_t g_dyna_target = 0;

void dyna_jump()
{
    precomp_block *block = blocks[PC->addr >> 12];
    precomp_instr *cur;

    if (!block || !block->block)
    {
        block = actual;
        cur = PC;
    }
    else
    {
        cur = block->block + ((PC->addr - block->start) >> 2);
    }

    g_dyna_target = cur->reg_cache_infos.need_map ? (uintptr_t)cur->reg_cache_infos.jump_wrapper
                                                  : (uintptr_t)(block->code + cur->local_addr);
}

static void (*dynarec_enter)(void (*code)()) = nullptr;

static void build_dynarec_enter()
{
    unsigned char *buf = (unsigned char *)malloc_exec(64);
    dynarec_enter = (void (*)(void (*)()))buf;

    unsigned char *saved_ptr = *inst_pointer;
    unsigned char **saved_inst = inst_pointer;
    const int32_t saved_len = code_length;
    const int32_t saved_max = max_code_length;

    inst_pointer = &buf;
    code_length = 0;
    max_code_length = 64;

    exec_write_begin();

    emit_sub_imm_x(A64_SP, A64_SP, 96);
    emit_str_x_off(A64_LR, A64_SP, 0);
    emit_str_x_off(A64_RB, A64_SP, 8);
    for (int32_t i = 0; i < A64_CACHE_COUNT; i++)
        emit_str_x_off(A64_CACHE_FIRST + i, A64_SP, (uint32_t)(16 + i * 8));

    emit_mov_reg_imm64(A64_RB, (uintptr_t)&reg[0]);
    emit_blr(A64_X0);

    for (int32_t i = 0; i < A64_CACHE_COUNT; i++)
        emit_ldr_x_off(A64_CACHE_FIRST + i, A64_SP, (uint32_t)(16 + i * 8));
    emit_ldr_x_off(A64_RB, A64_SP, 8);
    emit_ldr_x_off(A64_LR, A64_SP, 0);
    emit_add_imm_x(A64_SP, A64_SP, 96);
    emit_ret();

    __builtin___clear_cache((char *)dynarec_enter, (char *)dynarec_enter + code_length);
    exec_write_end();

    inst_pointer = saved_inst;
    *inst_pointer = saved_ptr;
    code_length = saved_len;
    max_code_length = saved_max;
}

namespace
{
jmp_buf g_dyna_ctx;
volatile bool g_dyna_stopped;
}

void dyna_start(void (*code)())
{
    core_executing = true;
    g_core->callbacks.core_executing_changed(core_executing);

    g_dyna_stopped = false;
    g_dyna_target = 0;

    if (!dynarec_enter) build_dynarec_enter();

    if (setjmp(g_dyna_ctx) == 0)
    {
        dynarec_enter(code);
    }
}

void dyna_stop()
{
    g_dyna_stopped = true;
    longjmp(g_dyna_ctx, 1);
}
