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

namespace
{
precomp_instr *last_access[A64_CACHE_COUNT];
precomp_instr *free_since[A64_CACHE_COUNT];
uintptr_t reg_content[A64_CACHE_COUNT];
bool dirty[A64_CACHE_COUNT];

int32_t host_reg(int32_t slot)
{
    return A64_CACHE_FIRST + slot;
}

uint32_t guest_offset(uintptr_t addr)
{
    return reg_offset((const void *)addr);
}
}

void init_cache(precomp_instr *start)
{
    for (int32_t i = 0; i < A64_CACHE_COUNT; i++)
    {
        last_access[i] = nullptr;
        free_since[i] = start;
        reg_content[i] = 0;
        dirty[i] = false;
    }
}

static void mark_span(int32_t slot, void *content)
{
    precomp_instr *last = last_access[slot] != nullptr ? last_access[slot] + 1 : free_since[slot];

    while (last <= dst)
    {
        last->reg_cache_infos.needed_registers[slot] = content;
        last++;
    }
}

void free_register(int32_t slot)
{
    mark_span(slot, (last_access[slot] != nullptr && dirty[slot]) ? (void *)reg_content[slot] : nullptr);

    if (last_access[slot] == nullptr)
    {
        free_since[slot] = dst + 1;
        return;
    }

    if (dirty[slot])
    {
        emit_str_x_off(host_reg(slot), A64_RB, guest_offset(reg_content[slot]));
        dirty[slot] = false;
    }

    last_access[slot] = nullptr;
    free_since[slot] = dst + 1;
}

void free_all_registers()
{
    for (int32_t i = 0; i < A64_CACHE_COUNT; i++)
    {
        if (last_access[i] != nullptr)
        {
            free_register(i);
            continue;
        }

        while (free_since[i] <= dst)
        {
            free_since[i]->reg_cache_infos.needed_registers[i] = nullptr;
            free_since[i]++;
        }
    }
}

static int32_t lru_slot()
{
    int32_t best = 0;
    for (int32_t i = 0; i < A64_CACHE_COUNT; i++)
    {
        if (last_access[i] == nullptr) return i;
        if (last_access[i] < last_access[best]) best = i;
    }
    return best;
}

static int32_t find_cached(uintptr_t addr)
{
    for (int32_t i = 0; i < A64_CACHE_COUNT; i++)
    {
        if (last_access[i] != nullptr && reg_content[i] == addr) return i;
    }
    return -1;
}

static bool cacheable(uintptr_t addr)
{
    return addr != (uintptr_t)&reg[0];
}

int32_t allocate_register(uintptr_t addr)
{
    const int32_t hit = find_cached(addr);
    if (hit >= 0)
    {
        mark_span(hit, (void *)reg_content[hit]);
        last_access[hit] = dst;
        return host_reg(hit);
    }

    const int32_t slot = lru_slot();
    free_register(slot);

    emit_ldr_x_off(host_reg(slot), A64_RB, guest_offset(addr));

    last_access[slot] = dst;
    reg_content[slot] = addr;
    dirty[slot] = false;
    return host_reg(slot);
}

int32_t allocate_register_w(uintptr_t addr)
{
    const int32_t hit = find_cached(addr);
    if (hit >= 0)
    {
        mark_span(hit, nullptr);
        last_access[hit] = dst;
        dirty[hit] = true;
        return host_reg(hit);
    }

    const int32_t slot = lru_slot();
    free_register(slot);

    last_access[slot] = dst;
    reg_content[slot] = addr;
    dirty[slot] = true;
    return host_reg(slot);
}

void simplify_access()
{
    dst->local_addr = code_length;
    for (int32_t i = 0; i < A64_CACHE_COUNT; i++) dst->reg_cache_infos.needed_registers[i] = nullptr;
}

static unsigned char *wrapper_alloc(size_t size)
{
    static unsigned char *pool = nullptr;
    static size_t used = 0;
    static constexpr size_t pool_size = 64 * 1024;

    if (!pool || used + size > pool_size)
    {
        pool = (unsigned char *)malloc_exec(pool_size);
        used = 0;
    }

    unsigned char *slot = pool + used;
    used += size;
    return slot;
}

void build_wrapper(precomp_instr *instr, precomp_block *block)
{
    static constexpr size_t max_size = (A64_CACHE_COUNT * 2 + 12) * 4;

    unsigned char *buf = wrapper_alloc(max_size);
    instr->reg_cache_infos.jump_wrapper = buf;

    unsigned char **saved_inst = inst_pointer;
    unsigned char *saved_ptr = *inst_pointer;
    const int32_t saved_len = code_length;
    const int32_t saved_max = max_code_length;

    inst_pointer = &buf;
    code_length = 0;
    max_code_length = (int32_t)max_size;

    exec_write_begin();

    for (int32_t i = 0; i < A64_CACHE_COUNT; i++)
    {
        void *content = instr->reg_cache_infos.needed_registers[i];
        if (content == nullptr) continue;

        emit_ldr_x_off(host_reg(i), A64_RB, guest_offset((uintptr_t)content));
    }

    emit_mov_reg_imm64(A64_IP0, (uintptr_t)&block->code);
    emit_ldr_x_off(A64_IP0, A64_IP0, 0);
    emit_mov_reg_imm64(A64_IP1, (uintptr_t)&instr->local_addr);
    emit_ldr_x_off(A64_IP1, A64_IP1, 0);
    emit_add(A64_IP0, A64_IP0, A64_IP1, true);
    emit_br(A64_IP0);

    __builtin___clear_cache((char *)instr->reg_cache_infos.jump_wrapper,
        (char *)instr->reg_cache_infos.jump_wrapper + code_length);
    exec_write_end();

    inst_pointer = saved_inst;
    *inst_pointer = saved_ptr;
    code_length = saved_len;
    max_code_length = saved_max;
}

void build_wrappers(precomp_instr *instr, int32_t start, int32_t end, precomp_block *block)
{
    for (int32_t i = start; i < end; i++)
    {
        instr[i].reg_cache_infos.need_map = 0;

        for (int32_t slot = 0; slot < A64_CACHE_COUNT; slot++)
        {
            if (instr[i].reg_cache_infos.needed_registers[slot] == nullptr) continue;

            instr[i].reg_cache_infos.need_map = 1;
            build_wrapper(&instr[i], block);
            break;
        }
    }
}
