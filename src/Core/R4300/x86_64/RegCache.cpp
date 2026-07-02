/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include "RegCache.hpp"
#include <R4300/R4300.hpp>
#include <R4300/Recomp.hpp>
#include <R4300/Recomph.hpp>

static uintptr_t reg_content[8];
static precomp_instr *last_access[8];
static precomp_instr *free_since[8];
static int32_t dirty[8];
static int32_t r64[8];
static uintptr_t r0;

void init_cache(precomp_instr *start)
{
    int32_t i;
    for (i = 0; i < 8; i++)
    {
        last_access[i] = NULL;
        free_since[i] = start;
    }
    r0 = (uintptr_t)reg;
}

void free_all_registers()
{
    int32_t i;
    for (i = 0; i < 8; i++)
    {
        if (last_access[i])
            free_register(i);
        else
        {
            while (free_since[i] <= dst)
            {
                free_since[i]->reg_cache_infos.needed_registers[i] = NULL;
                free_since[i]++;
            }
        }
    }
}

// this function frees a specific X86 GPR
void free_register(int32_t reg)
{
    precomp_instr *last;

    if (last_access[reg] != NULL && r64[reg] != -1 && (uintptr_t)reg_content[reg] != (uintptr_t)reg_content[r64[reg]] - 4)
    {
        free_register(r64[reg]);
        return;
    }

    if (last_access[reg] != NULL)
        last = last_access[reg] + 1;
    else
        last = free_since[reg];

    while (last <= dst)
    {
        if (last_access[reg] != NULL && dirty[reg])
            last->reg_cache_infos.needed_registers[reg] = (void*)reg_content[reg];
        else
            last->reg_cache_infos.needed_registers[reg] = NULL;

        if (last_access[reg] != NULL && r64[reg] != -1)
        {
            if (dirty[r64[reg]])
                last->reg_cache_infos.needed_registers[r64[reg]] = (void*)reg_content[r64[reg]];
            else
                last->reg_cache_infos.needed_registers[r64[reg]] = NULL;
        }

        last++;
    }
    if (last_access[reg] == NULL)
    {
        free_since[reg] = dst + 1;
        return;
    }

    if (dirty[reg])
    {
        if (r64[reg] == -1)
        {
            // Standalone 32-bit value: the x86 register holds a 32-bit result
            // whose canonical 64-bit form is its sign extension. 32-bit x86 ALU
            // ops zero-extend into the upper 32 bits, so storing the raw 64-bit
            // register would zero-extend (wrong). Sign-extend first.
            movsxd_reg64_reg32(reg, reg);
            mov_m64_reg64((void*)reg_content[reg], reg);
        }
        else
        {
            // 64-bit pair: low and high halves live in two separate x86
            // registers. Write each as an independent 32-bit store so the high
            // 32 bits survive (a single 64-bit store of the low register would
            // zero reg[]'s high dword and drop the high half entirely).
            mov_m32_reg32((void*)reg_content[reg], reg);
            mov_m32_reg32((void*)reg_content[r64[reg]], r64[reg]);
            dirty[r64[reg]] = 0;
        }
        dirty[reg] = 0;
    }
    last_access[reg] = NULL;
    free_since[reg] = dst + 1;
    if (r64[reg] != -1)
    {
        last_access[r64[reg]] = NULL;
        free_since[r64[reg]] = dst + 1;
    }
}

int32_t lru_register()
{
    uintptr_t oldest_access = static_cast<uintptr_t>(-1);
    int32_t i, reg = 0;
    for (i = 0; i < 8; i++)
    {
        if (i != ESP && (uintptr_t)last_access[i] < oldest_access)
        {
            oldest_access = (uintptr_t)last_access[i];
            reg = i;
        }
    }
    return reg;
}

int32_t lru_register_exc1(int32_t exc1)
{
    uintptr_t oldest_access = static_cast<uintptr_t>(-1);
    int32_t i, reg = 0;
    for (i = 0; i < 8; i++)
    {
        if (i != ESP && i != exc1 && (uintptr_t)last_access[i] < oldest_access)
        {
            oldest_access = (uintptr_t)last_access[i];
            reg = i;
        }
    }
    return reg;
}

// this function finds a register to put the data contained in addr,
// if there was another value before it's cleanly removed of the
// register cache. After that, the register number is returned.
// If data are already cached, the function only returns the register number
int32_t allocate_register(uintptr_t addr)
{
    uintptr_t oldest_access = UINTPTR_MAX;
    int32_t reg = 0, i;

    // is it already cached ?
    if (addr != 0)
    {
        for (i = 0; i < 8; i++)
        {
            if (last_access[i] != NULL && (uintptr_t)reg_content[i] == addr)
            {
                precomp_instr *last = last_access[i] + 1;

                while (last <= dst)
                {
                    last->reg_cache_infos.needed_registers[i] = (void*)reg_content[i];
                    last++;
                }
                last_access[i] = dst;
                if (r64[i] != -1)
                {
                    last = last_access[r64[i]] + 1;

                    while (last <= dst)
                    {
                        last->reg_cache_infos.needed_registers[r64[i]] = (void*)reg_content[r64[i]];
                        last++;
                    }
                    last_access[r64[i]] = dst;
                }

                return i;
            }
        }
    }

    // if it's not cached, we take the least recently used register
    for (i = 0; i < 8; i++)
    {
        if (i != ESP && (uintptr_t)last_access[i] < oldest_access)
        {
            oldest_access = (uintptr_t)last_access[i];
            reg = i;
        }
    }

    if (last_access[reg])
        free_register(reg);
    else
    {
        while (free_since[reg] <= dst)
        {
            free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
            free_since[reg]++;
        }
    }

    last_access[reg] = dst;
    reg_content[reg] = addr;
    dirty[reg] = 0;
    r64[reg] = -1;

    if (addr != 0)
    {
        if (addr == r0 || addr == r0 + 1)
            xor_reg32_reg32(reg, reg);
        else
            mov_reg64_m64(reg, (void*)addr);
    }

    return reg;
}

// this function is similar to allocate_register except it loads
// a 64 bits value, and return the register number of the LSB part
int32_t allocate_64_register1(uintptr_t addr)
{
    int32_t reg1, reg2, i;

    // is it already cached as a 32 bits value ?
    for (i = 0; i < 8; i++)
    {
        if (last_access[i] != NULL && reg_content[i] == addr)
        {
            if (r64[i] == -1)
            {
                allocate_register(addr);
                reg2 = allocate_register(dirty[i] ? 0 : addr + 4);
                r64[i] = reg2;
                r64[reg2] = i;

                if (dirty[i])
                {
                    reg_content[reg2] = addr + 4;
                    dirty[reg2] = 1;
                    mov_reg32_reg32(reg2, i);
                    sar_reg32_imm8(reg2, 31);
                }

                return i;
            }
        }
    }

    reg1 = allocate_register(addr);
    reg2 = allocate_register(addr + 4);
    r64[reg1] = reg2;
    r64[reg2] = reg1;

    return reg1;
}

// this function is similar to allocate_register except it loads
// a 64 bits value, and return the register number of the MSB part
int32_t allocate_64_register2(uintptr_t addr)
{
    int32_t reg1, reg2, i;

    // is it already cached as a 32 bits value ?
    for (i = 0; i < 8; i++)
    {
        if (last_access[i] != NULL && reg_content[i] == addr)
        {
            if (r64[i] == -1)
            {
                allocate_register(addr);
                reg2 = allocate_register(dirty[i] ? 0 : addr + 4);
                r64[i] = reg2;
                r64[reg2] = i;

                if (dirty[i])
                {
                    reg_content[reg2] = addr + 4;
                    dirty[reg2] = 1;
                    mov_reg32_reg32(reg2, i);
                    sar_reg32_imm8(reg2, 31);
                }

                return reg2;
            }
        }
    }

    reg1 = allocate_register(addr);
    reg2 = allocate_register(addr + 4);
    r64[reg1] = reg2;
    r64[reg2] = reg1;

    return reg2;
}

// this function checks if the data located at addr are cached in a register
// and then, it returns 1  if it's a 64 bit value
//                      0  if it's a 32 bit value
//                      -1 if it's not cached
int32_t is64(uintptr_t addr)
{
    int32_t i;
    for (i = 0; i < 8; i++)
    {
        if (last_access[i] != NULL && reg_content[i] == addr)
        {
            if (r64[i] == -1) return 0;
            return 1;
        }
    }
    return -1;
}

int32_t allocate_register_w(uintptr_t addr)
{
    uintptr_t oldest_access = UINTPTR_MAX;
    int32_t reg = 0, i;

    // is it already cached ?
    for (i = 0; i < 8; i++)
    {
        if (last_access[i] != NULL && reg_content[i] == addr)
        {
            precomp_instr *last = last_access[i] + 1;

            while (last <= dst)
            {
                last->reg_cache_infos.needed_registers[i] = NULL;
                last++;
            }
            last_access[i] = dst;
            dirty[i] = 1;
            if (r64[i] != -1)
            {
                last = last_access[r64[i]] + 1;
                while (last <= dst)
                {
                    last->reg_cache_infos.needed_registers[r64[i]] = NULL;
                    last++;
                }
                free_since[r64[i]] = dst + 1;
                last_access[r64[i]] = NULL;
                r64[i] = -1;
            }

            return i;
        }
    }

    // if it's not cached, we take the least recently used register
    for (i = 0; i < 8; i++)
    {
        if (i != ESP && (uintptr_t)last_access[i] < oldest_access)
        {
            oldest_access = (uintptr_t)last_access[i];
            reg = i;
        }
    }

    if (last_access[reg])
        free_register(reg);
    else
    {
        while (free_since[reg] <= dst)
        {
            free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
            free_since[reg]++;
        }
    }

    last_access[reg] = dst;
    reg_content[reg] = addr;
    dirty[reg] = 1;
    r64[reg] = -1;

    return reg;
}

int32_t allocate_64_register1_w(uintptr_t addr)
{
    int32_t reg1, reg2, i;

    // is it already cached as a 32 bits value ?
    for (i = 0; i < 8; i++)
    {
        if (last_access[i] != NULL && reg_content[i] == addr)
        {
            if (r64[i] == -1)
            {
                allocate_register_w(addr);
                reg2 = lru_register();
                if (last_access[reg2]) free_register(reg2);
                r64[i] = reg2;
                r64[reg2] = i;
                last_access[reg2] = dst;

                reg_content[reg2] = addr + 4;
                dirty[reg2] = 1;
                mov_reg32_reg32(reg2, i);
                sar_reg32_imm8(reg2, 31);

                return i;
            }
            else
            {
                last_access[i] = dst;
                last_access[r64[i]] = dst;
                dirty[i] = dirty[r64[i]] = 1;
                return i;
            }
        }
    }

    reg1 = allocate_register_w(addr);
    reg2 = lru_register();
    if (last_access[reg2])
        free_register(reg2);
    else
    {
        while (free_since[reg2] <= dst)
        {
            free_since[reg2]->reg_cache_infos.needed_registers[reg2] = NULL;
            free_since[reg2]++;
        }
    }
    r64[reg1] = reg2;
    r64[reg2] = reg1;
    last_access[reg2] = dst;
    reg_content[reg2] = addr + 4;
    dirty[reg2] = 1;

    return reg1;
}

int32_t allocate_64_register2_w(uintptr_t addr)
{
    int32_t reg1, reg2, i;

    // is it already cached as a 32 bits value ?
    for (i = 0; i < 8; i++)
    {
        if (last_access[i] != NULL && reg_content[i] == addr)
        {
            if (r64[i] == -1)
            {
                allocate_register_w(addr);
                reg2 = lru_register();
                if (last_access[reg2]) free_register(reg2);
                r64[i] = reg2;
                r64[reg2] = i;
                last_access[reg2] = dst;

                reg_content[reg2] = addr + 4;
                dirty[reg2] = 1;
                mov_reg32_reg32(reg2, i);
                sar_reg32_imm8(reg2, 31);

                return reg2;
            }
            else
            {
                last_access[i] = dst;
                last_access[r64[i]] = dst;
                dirty[i] = dirty[r64[i]] = 1;
                return r64[i];
            }
        }
    }

    reg1 = allocate_register_w(addr);
    reg2 = lru_register();
    if (last_access[reg2])
        free_register(reg2);
    else
    {
        while (free_since[reg2] <= dst)
        {
            free_since[reg2]->reg_cache_infos.needed_registers[reg2] = NULL;
            free_since[reg2]++;
        }
    }
    r64[reg1] = reg2;
    r64[reg2] = reg1;
    last_access[reg2] = dst;
    reg_content[reg2] = addr + 4;
    dirty[reg2] = 1;

    return reg2;
}

void set_register_state(int32_t reg, uintptr_t addr, int32_t d)
{
    last_access[reg] = dst;
    reg_content[reg] = addr;
    r64[reg] = -1;
    dirty[reg] = d;
}

void set_64_register_state(int32_t reg1, int32_t reg2, uintptr_t addr, int32_t d)
{
    last_access[reg1] = dst;
    last_access[reg2] = dst;
    reg_content[reg1] = addr;
    reg_content[reg2] = addr + 4;
    r64[reg1] = reg2;
    r64[reg2] = reg1;
    dirty[reg1] = d;
    dirty[reg2] = d;
}

void lock_register(int32_t reg)
{
    free_register(reg);
    last_access[reg] = (precomp_instr *)0xFFFFFFFF;
    reg_content[reg] = 0;
}

void unlock_register(int32_t reg)
{
    last_access[reg] = NULL;
}

void force_32(int32_t reg)
{
    if (r64[reg] != -1)
    {
        precomp_instr *last = last_access[reg] + 1;

        while (last <= dst)
        {
            if (dirty[reg])
                last->reg_cache_infos.needed_registers[reg] = (void*)reg_content[reg];
            else
                last->reg_cache_infos.needed_registers[reg] = NULL;

            if (dirty[r64[reg]])
                last->reg_cache_infos.needed_registers[r64[reg]] = (void*)reg_content[r64[reg]];
            else
                last->reg_cache_infos.needed_registers[r64[reg]] = NULL;

            last++;
        }

        if (dirty[reg])
        {
            // Collapsing a 64-bit pair down to a 32-bit value: the surviving low
            // half must be sign-extended (its former high half is discarded).
            // movsxd keeps the low 32 bits intact and sets the upper 32 to the
            // sign, so reg stays a valid live register afterwards.
            movsxd_reg64_reg32(reg, reg);
            mov_m64_reg64((void*)reg_content[reg], reg);
            dirty[reg] = 0;
        }
        last_access[r64[reg]] = NULL;
        free_since[r64[reg]] = dst + 1;
        r64[reg] = -1;
    }
}

void allocate_register_manually(int32_t reg, uintptr_t addr)
{
    int32_t i;

    if (last_access[reg] != NULL && reg_content[reg] == addr)
    {
        precomp_instr *last = last_access[reg] + 1;

        while (last <= dst)
        {
            last->reg_cache_infos.needed_registers[reg] = (void*)reg_content[reg];
            last++;
        }
        last_access[reg] = dst;
        if (r64[reg] != -1)
        {
            last = last_access[r64[reg]] + 1;

            while (last <= dst)
            {
                last->reg_cache_infos.needed_registers[r64[reg]] = (void*)reg_content[r64[reg]];
                last++;
            }
            last_access[r64[reg]] = dst;
        }
        return;
    }

    if (last_access[reg])
        free_register(reg);
    else
    {
        while (free_since[reg] <= dst)
        {
            free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
            free_since[reg]++;
        }
    }

    // is it already cached ?
    for (i = 0; i < 8; i++)
    {
        if (last_access[i] != NULL && reg_content[i] == addr)
        {
            precomp_instr *last = last_access[i] + 1;

            while (last <= dst)
            {
                last->reg_cache_infos.needed_registers[i] = (void*)reg_content[i];
                last++;
            }
            last_access[i] = dst;
            if (r64[i] != -1)
            {
                last = last_access[r64[i]] + 1;

                while (last <= dst)
                {
                    last->reg_cache_infos.needed_registers[r64[i]] = (void*)reg_content[r64[i]];
                    last++;
                }
                last_access[r64[i]] = dst;
            }

            mov_reg32_reg32(reg, i);
            last_access[reg] = dst;
            r64[reg] = r64[i];
            if (r64[reg] != -1) r64[r64[reg]] = reg;
            dirty[reg] = dirty[i];
            reg_content[reg] = reg_content[i];
            free_since[i] = dst + 1;
            last_access[i] = NULL;

            return;
        }
    }

    last_access[reg] = dst;
    reg_content[reg] = addr;
    dirty[reg] = 0;
    r64[reg] = -1;

    if (addr != 0)
    {
        if (addr == r0 || addr == r0 + 1)
            xor_reg32_reg32(reg, reg);
        else
            mov_reg64_m64(reg, (void*)addr);
    }
}

void allocate_register_manually_w(int32_t reg, uintptr_t addr, int32_t load)
{
    int32_t i;

    if (last_access[reg] != NULL && reg_content[reg] == addr)
    {
        precomp_instr *last = last_access[reg] + 1;

        while (last <= dst)
        {
            last->reg_cache_infos.needed_registers[reg] = (void*)reg_content[reg];
            last++;
        }
        last_access[reg] = dst;

        if (r64[reg] != -1)
        {
            last = last_access[r64[reg]] + 1;

            while (last <= dst)
            {
                last->reg_cache_infos.needed_registers[r64[reg]] = (void*)reg_content[r64[reg]];
                last++;
            }
            last_access[r64[reg]] = NULL;
            free_since[r64[reg]] = dst + 1;
            r64[reg] = -1;
        }
        dirty[reg] = 1;
        return;
    }

    if (last_access[reg])
        free_register(reg);
    else
    {
        while (free_since[reg] <= dst)
        {
            free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
            free_since[reg]++;
        }
    }

    // is it already cached ?
    for (i = 0; i < 8; i++)
    {
        if (last_access[i] != NULL && reg_content[i] == addr)
        {
            precomp_instr *last = last_access[i] + 1;

            while (last <= dst)
            {
                last->reg_cache_infos.needed_registers[i] = (void*)reg_content[i];
                last++;
            }
            last_access[i] = dst;
            if (r64[i] != -1)
            {
                last = last_access[r64[i]] + 1;
                while (last <= dst)
                {
                    last->reg_cache_infos.needed_registers[r64[i]] = NULL;
                    last++;
                }
                free_since[r64[i]] = dst + 1;
                last_access[r64[i]] = NULL;
                r64[i] = -1;
            }

            if (load) mov_reg32_reg32(reg, i);
            last_access[reg] = dst;
            dirty[reg] = 1;
            r64[reg] = -1;
            reg_content[reg] = reg_content[i];
            free_since[i] = dst + 1;
            last_access[i] = NULL;

            return;
        }
    }

    last_access[reg] = dst;
    reg_content[reg] = addr;
    dirty[reg] = 1;
    r64[reg] = -1;

    if (addr != 0 && load)
    {
        if (addr == r0 || addr == r0 + 1)
            xor_reg32_reg32(reg, reg);
        else
            mov_reg64_m64(reg, (void*)addr);
    }
}

// 0x81 0xEC 0x4 0x0 0x0 0x0  sub esp, 4
// 0xA1            0xXXXXXXXX mov eax, XXXXXXXX (&code start)
// 0x05            0xXXXXXXXX add eax, XXXXXXXX (local_addr)
// 0x89 0x04 0x24             mov [esp], eax
// 0x8B (reg<<3)|5 0xXXXXXXXX mov eax, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov ebx, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov ecx, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov edx, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov ebp, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov esi, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov edi, [XXXXXXXX]
// 0xC3 ret
// total : 62 bytes
void build_wrapper(precomp_instr *instr, unsigned char *code, precomp_block *block)
{
    int32_t i;
    int32_t j = 0;

    // x64 wrapper: reload needed registers, then jump to block->code + local_addr.
    // We use R10 as scratch for memory loads and R11 for the jump target.
    //
    // The wrapper must NOT touch RSP. It is always entered via a `jmp` with the
    // stack pointer already at the block's canonical entry value (RSP%16==8):
    //   - the passe2 need_map stub jumps here (mov rdx, wrapper; jmp rdx),
    //   - genjr/genjalr need_map fast paths jump here (jmp rax),
    //   - the dyna_jump need_map thunk already does `add rsp, 8` to undo the
    //     outer call_reg64's push r11 before jumping here.
    // A `sub rsp, 8` here would misalign the stack by 8 for the whole block, so
    // that the next call_reg64 lands its C callee at RSP%16==0 — crashing any
    // callee that uses 16-byte-aligned (AVX) stack spills, e.g. std::format.

    for (i = 0; i < 8; i++)
    {
        if (instr->reg_cache_infos.needed_registers[i] != NULL)
        {
            // mov r10, imm64
            code[j++] = 0x49;
            code[j++] = 0xBA;
            *((uintptr_t *)&code[j]) = (uintptr_t)instr->reg_cache_infos.needed_registers[i];
            j += 8;

            // mov reg32_i, [r10]
            code[j++] = 0x41;
            code[j++] = 0x8B;
            code[j++] = (i << 3) | 0x02;
        }
    }

    // Compute the jump target at RUNTIME from the live struct fields, rather
    // than baking an absolute (block->code + local_addr) here. That absolute
    // value goes stale: block->code is realloc'd (moved) whenever the code
    // buffer grows (grow_buffer/realloc_exec), and instr->local_addr is
    // recomputed on every re-recompilation. A baked target then points into a
    // superseded (deferred-freed) buffer at a stale offset — landing in old or
    // zeroed memory. The *addresses* of block->code and instr->local_addr are
    // stable (both structs persist for the block's lifetime), so we load
    // through them each time the wrapper runs. R10 is scratch; R11 is target.

    // mov r10, imm64(&block->code)
    code[j++] = 0x49;
    code[j++] = 0xBA;
    *((uintptr_t *)&code[j]) = (uintptr_t)&block->code;
    j += 8;

    // mov r11, [r10]        (r11 = block->code, live)
    code[j++] = 0x4D;
    code[j++] = 0x8B;
    code[j++] = 0x1A;

    // mov r10, imm64(&instr->local_addr)
    code[j++] = 0x49;
    code[j++] = 0xBA;
    *((uintptr_t *)&code[j]) = (uintptr_t)&instr->local_addr;
    j += 8;

    // add r11, [r10]        (r11 += instr->local_addr, live)
    code[j++] = 0x4D;
    code[j++] = 0x03;
    code[j++] = 0x1A;

    // jmp r11
    code[j++] = 0x49;
    code[j++] = 0xFF;
    code[j++] = 0xE3;
}

void build_wrappers(precomp_instr *instr, int32_t start, int32_t end, precomp_block *block)
{
    int32_t i, reg;
    ;
    for (i = start; i < end; i++)
    {
        instr[i].reg_cache_infos.need_map = 0;
        for (reg = 0; reg < 8; reg++)
        {
            if (instr[i].reg_cache_infos.needed_registers[reg] != NULL)
            {
                instr[i].reg_cache_infos.need_map = 1;
                build_wrapper(&instr[i], instr[i].reg_cache_infos.jump_wrapper, block);
                break;
            }
        }
    }
}

void simplify_access()
{
    int32_t i;
    dst->local_addr = code_length;
    for (i = 0; i < 8; i++) dst->reg_cache_infos.needed_registers[i] = NULL;
}
