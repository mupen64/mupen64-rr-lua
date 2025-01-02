/**
 * Mupen64 - regcache.c
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

#include <stdio.h>

#include "../r4300.h"
#include "../recomp.h"
#include "../recomph.h"
#include "regcache.h"

static uint32_t* reg_content[8];
static precomp_instr* last_access[8];
static precomp_instr* free_since[8];
static int32_t dirty[8];
static int32_t r64[8];
static uint32_t* r0;

void init_cache(precomp_instr* start)
{
    int32_t i;
    for (i = 0; i < 8; i++)
    {
        last_access[i] = NULL;
        free_since[i] = start;
    }
    r0 = (uint32_t*)reg;
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
    precomp_instr* last;

    if (last_access[reg] != NULL && r64[reg] != -1 && (int32_t)reg_content[reg] != (int32_t)reg_content[r64[reg]] - 4)
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
            last->reg_cache_infos.needed_registers[reg] = reg_content[reg];
        else
            last->reg_cache_infos.needed_registers[reg] = NULL;

        if (last_access[reg] != NULL && r64[reg] != -1)
        {
            if (dirty[r64[reg]])
                last->reg_cache_infos.needed_registers[r64[reg]] = reg_content[r64[reg]];
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
        mov_m32_reg32(reg_content[reg], reg);
        if (r64[reg] == -1)
        {
            sar_reg32_imm8(reg, 31);
            mov_m32_reg32((uint32_t*)reg_content[reg] + 1, reg);
        }
        else
            mov_m32_reg32(reg_content[r64[reg]], r64[reg]);
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
    uint32_t oldest_access = 0xFFFFFFFF;
    int32_t i, reg = 0;
    for (i = 0; i < 8; i++)
    {
        if (i != ESP && (uint32_t)last_access[i] < oldest_access)
        {
            oldest_access = (int32_t)last_access[i];
            reg = i;
        }
    }
    return reg;
}

int32_t lru_register_exc1(int32_t exc1)
{
    uint32_t oldest_access = 0xFFFFFFFF;
    int32_t i, reg = 0;
    for (i = 0; i < 8; i++)
    {
        if (i != ESP && i != exc1 && (uint32_t)last_access[i] < oldest_access)
        {
            oldest_access = (int32_t)last_access[i];
            reg = i;
        }
    }
    return reg;
}

// this function finds a register to put the data contained in addr,
// if there was another value before it's cleanly removed of the
// register cache. After that, the register number is returned.
// If data are already cached, the function only returns the register number
int32_t allocate_register(uint32_t* addr)
{
    uint32_t oldest_access = 0xFFFFFFFF;
    int32_t reg = 0, i;

    // is it already cached ?
    if (addr != NULL)
    {
        for (i = 0; i < 8; i++)
        {
            if (last_access[i] != NULL && reg_content[i] == addr)
            {
                precomp_instr* last = last_access[i] + 1;

                while (last <= dst)
                {
                    last->reg_cache_infos.needed_registers[i] = reg_content[i];
                    last++;
                }
                last_access[i] = dst;
                if (r64[i] != -1)
                {
                    last = last_access[r64[i]] + 1;

                    while (last <= dst)
                    {
                        last->reg_cache_infos.needed_registers[r64[i]] = reg_content[r64[i]];
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
        if (i != ESP && (uint32_t)last_access[i] < oldest_access)
        {
            oldest_access = (int32_t)last_access[i];
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

    if (addr != NULL)
    {
        if (addr == r0 || addr == r0 + 1)
            xor_reg32_reg32(reg, reg);
        else
            mov_reg32_m32(reg, addr);
    }

    return reg;
}

// this function is similar to allocate_register except it loads
// a 64 bits value, and return the register number of the LSB part
int32_t allocate_64_register1(uint32_t* addr)
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
                reg2 = allocate_register(dirty[i] ? NULL : addr + 1);
                r64[i] = reg2;
                r64[reg2] = i;

                if (dirty[i])
                {
                    reg_content[reg2] = addr + 1;
                    dirty[reg2] = 1;
                    mov_reg32_reg32(reg2, i);
                    sar_reg32_imm8(reg2, 31);
                }

                return i;
            }
        }
    }

    reg1 = allocate_register(addr);
    reg2 = allocate_register(addr + 1);
    r64[reg1] = reg2;
    r64[reg2] = reg1;

    return reg1;
}

// this function is similar to allocate_register except it loads
// a 64 bits value, and return the register number of the MSB part
int32_t allocate_64_register2(uint32_t* addr)
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
                reg2 = allocate_register(dirty[i] ? NULL : addr + 1);
                r64[i] = reg2;
                r64[reg2] = i;

                if (dirty[i])
                {
                    reg_content[reg2] = addr + 1;
                    dirty[reg2] = 1;
                    mov_reg32_reg32(reg2, i);
                    sar_reg32_imm8(reg2, 31);
                }

                return reg2;
            }
        }
    }

    reg1 = allocate_register(addr);
    reg2 = allocate_register(addr + 1);
    r64[reg1] = reg2;
    r64[reg2] = reg1;

    return reg2;
}

// this function checks if the data located at addr are cached in a register
// and then, it returns 1  if it's a 64 bit value
//                      0  if it's a 32 bit value
//                      -1 if it's not cached
int32_t is64(uint32_t* addr)
{
    int32_t i;
    for (i = 0; i < 8; i++)
    {
        if (last_access[i] != NULL && reg_content[i] == addr)
        {
            if (r64[i] == -1)
                return 0;
            return 1;
        }
    }
    return -1;
}

int32_t allocate_register_w(uint32_t* addr)
{
    uint32_t oldest_access = 0xFFFFFFFF;
    int32_t reg = 0, i;

    // is it already cached ?
    for (i = 0; i < 8; i++)
    {
        if (last_access[i] != NULL && reg_content[i] == addr)
        {
            precomp_instr* last = last_access[i] + 1;

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
        if (i != ESP && (uint32_t)last_access[i] < oldest_access)
        {
            oldest_access = (int32_t)last_access[i];
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

int32_t allocate_64_register1_w(uint32_t* addr)
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
                if (last_access[reg2])
                    free_register(reg2);
                r64[i] = reg2;
                r64[reg2] = i;
                last_access[reg2] = dst;

                reg_content[reg2] = addr + 1;
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
    reg_content[reg2] = addr + 1;
    dirty[reg2] = 1;

    return reg1;
}

int32_t allocate_64_register2_w(uint32_t* addr)
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
                if (last_access[reg2])
                    free_register(reg2);
                r64[i] = reg2;
                r64[reg2] = i;
                last_access[reg2] = dst;

                reg_content[reg2] = addr + 1;
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
    reg_content[reg2] = addr + 1;
    dirty[reg2] = 1;

    return reg2;
}

void set_register_state(int32_t reg, uint32_t* addr, int32_t d)
{
    last_access[reg] = dst;
    reg_content[reg] = addr;
    r64[reg] = -1;
    dirty[reg] = d;
}

void set_64_register_state(int32_t reg1, int32_t reg2, uint32_t* addr, int32_t d)
{
    last_access[reg1] = dst;
    last_access[reg2] = dst;
    reg_content[reg1] = addr;
    reg_content[reg2] = addr + 1;
    r64[reg1] = reg2;
    r64[reg2] = reg1;
    dirty[reg1] = d;
    dirty[reg2] = d;
}

void lock_register(int32_t reg)
{
    free_register(reg);
    last_access[reg] = (precomp_instr*)0xFFFFFFFF;
    reg_content[reg] = NULL;
}

void unlock_register(int32_t reg) { last_access[reg] = NULL; }

void force_32(int32_t reg)
{
    if (r64[reg] != -1)
    {
        precomp_instr* last = last_access[reg] + 1;

        while (last <= dst)
        {
            if (dirty[reg])
                last->reg_cache_infos.needed_registers[reg] = reg_content[reg];
            else
                last->reg_cache_infos.needed_registers[reg] = NULL;

            if (dirty[r64[reg]])
                last->reg_cache_infos.needed_registers[r64[reg]] = reg_content[r64[reg]];
            else
                last->reg_cache_infos.needed_registers[r64[reg]] = NULL;

            last++;
        }

        if (dirty[reg])
        {
            mov_m32_reg32(reg_content[reg], reg);
            mov_m32_reg32(reg_content[r64[reg]], r64[reg]);
            dirty[reg] = 0;
        }
        last_access[r64[reg]] = NULL;
        free_since[r64[reg]] = dst + 1;
        r64[reg] = -1;
    }
}

void allocate_register_manually(int32_t reg, uint32_t* addr)
{
    int32_t i;

    if (last_access[reg] != NULL && reg_content[reg] == addr)
    {
        precomp_instr* last = last_access[reg] + 1;

        while (last <= dst)
        {
            last->reg_cache_infos.needed_registers[reg] = reg_content[reg];
            last++;
        }
        last_access[reg] = dst;
        if (r64[reg] != -1)
        {
            last = last_access[r64[reg]] + 1;

            while (last <= dst)
            {
                last->reg_cache_infos.needed_registers[r64[reg]] = reg_content[r64[reg]];
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
            precomp_instr* last = last_access[i] + 1;

            while (last <= dst)
            {
                last->reg_cache_infos.needed_registers[i] = reg_content[i];
                last++;
            }
            last_access[i] = dst;
            if (r64[i] != -1)
            {
                last = last_access[r64[i]] + 1;

                while (last <= dst)
                {
                    last->reg_cache_infos.needed_registers[r64[i]] = reg_content[r64[i]];
                    last++;
                }
                last_access[r64[i]] = dst;
            }

            mov_reg32_reg32(reg, i);
            last_access[reg] = dst;
            r64[reg] = r64[i];
            if (r64[reg] != -1)
                r64[r64[reg]] = reg;
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

    if (addr != NULL)
    {
        if (addr == r0 || addr == r0 + 1)
            xor_reg32_reg32(reg, reg);
        else
            mov_reg32_m32(reg, addr);
    }
}

void allocate_register_manually_w(int32_t reg, uint32_t* addr, int32_t load)
{
    int32_t i;

    if (last_access[reg] != NULL && reg_content[reg] == addr)
    {
        precomp_instr* last = last_access[reg] + 1;

        while (last <= dst)
        {
            last->reg_cache_infos.needed_registers[reg] = reg_content[reg];
            last++;
        }
        last_access[reg] = dst;

        if (r64[reg] != -1)
        {
            last = last_access[r64[reg]] + 1;

            while (last <= dst)
            {
                last->reg_cache_infos.needed_registers[r64[reg]] = reg_content[r64[reg]];
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
            precomp_instr* last = last_access[i] + 1;

            while (last <= dst)
            {
                last->reg_cache_infos.needed_registers[i] = reg_content[i];
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

            if (load)
                mov_reg32_reg32(reg, i);
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

    if (addr != NULL && load)
    {
        if (addr == r0 || addr == r0 + 1)
            xor_reg32_reg32(reg, reg);
        else
            mov_reg32_m32(reg, addr);
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
void build_wrapper(precomp_instr* instr, unsigned char* code, precomp_block* block)
{
    int32_t i;
    int32_t j = 0;

    //--
    /*code[j++] = 0xB8;
    *((uint32_t*)&code[j]) = (uint32_t)(fake);
    j+=4;
    code[j++] = 0xFF;
    code[j++] = 0xD0;*/
    //--

    code[j++] = 0x81;
    code[j++] = 0xEC;
    code[j++] = 0x04;
    code[j++] = 0x00;
    code[j++] = 0x00;
    code[j++] = 0x00;

    code[j++] = 0xA1;
    *((uint32_t*)&code[j]) = (uint32_t)(&block->code);
    j += 4;

    code[j++] = 0x05;
    *((uint32_t*)&code[j]) = (uint32_t)instr->local_addr;
    j += 4;

    code[j++] = 0x89;
    code[j++] = 0x04;
    code[j++] = 0x24;

    for (i = 0; i < 8; i++)
    {
        if (instr->reg_cache_infos.needed_registers[i] != NULL)
        {
            code[j++] = 0x8B;
            code[j++] = (i << 3) | 5;
            *((uint32_t*)&code[j]) = (uint32_t)instr->reg_cache_infos.needed_registers[i];
            j += 4;
        }
    }

    code[j++] = 0xC3;
}

void build_wrappers(precomp_instr* instr, int32_t start, int32_t end, precomp_block* block)
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
    for (i = 0; i < 8; i++)
        dst->reg_cache_infos.needed_registers[i] = NULL;
}
