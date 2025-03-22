/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "tlb.h"
#include "memory.h"
#include <xxhash/xxh64.h>
#include <Core.h>
#include <r4300/exception.h>
#include <r4300/interrupt.h>
#include <r4300/macros.h>
#include <r4300/ops.h>
#include <r4300/r4300.h>
#include <r4300/recomph.h>
#include <r4300/rom.h>

uint32_t tlb_LUT_r[0x100000];
uint32_t tlb_LUT_w[0x100000];
extern uint32_t interp_addr;
int32_t jump_marker = 0;

uint32_t virtual_to_physical_address(uint32_t addresse, int32_t w)
{
    if (addresse >= 0x7f000000 && addresse < 0x80000000) // golden eye hack (it uses TLB a lot)
    {
        if (ROM_HEADER.CRC1 == sl(0xDCBC50D1)) // US
            return 0xb0034b30 + (addresse & 0xFFFFFF);
        if (ROM_HEADER.CRC1 == sl(0x0414CA61)) // E
            return 0xb00329f0 + (addresse & 0xFFFFFF);
        if (ROM_HEADER.CRC1 == sl(0xA24F4CF1)) // J
            return 0xb0034b70 + (addresse & 0xFFFFFF);
    }
    if (w == 1)
    {
        if (tlb_LUT_w[addresse >> 12])
            return (tlb_LUT_w[addresse >> 12] & 0xFFFFF000) | (addresse & 0xFFF);
    }
    else
    {
        if (tlb_LUT_r[addresse >> 12])
            return (tlb_LUT_r[addresse >> 12] & 0xFFFFF000) | (addresse & 0xFFF);
    }
    TLB_refill_exception(addresse, w);
    return 0x00000000;
}

int32_t probe_nop(uint32_t address)
{
    uint32_t a;
    if (address < 0x80000000 || address > 0xc0000000)
    {
        if (tlb_LUT_r[address >> 12])
            a = (tlb_LUT_r[address >> 12] & 0xFFFFF000) | (address & 0xFFF);
        else
            return 0;
    }
    else
        a = address;

    if (a >= 0xa4000000 && a < 0xa4001000)
    {
        if (!SP_DMEM[(a & 0xFFF) / 4])
            return 1;
        else
            return 0;
    }
    else if (a >= 0x80000000 && a < 0x80800000)
    {
        if (!rdram[(a & 0x7FFFFF) / 4])
            return 1;
        else
            return 0;
    }
    else
        return 0;
}

void TLBR()
{
    int32_t index;
    index = core_Index & 0x1F;
    core_PageMask = tlb_e[index].mask << 13;
    core_EntryHi = ((tlb_e[index].vpn2 << 13) | tlb_e[index].asid);
    core_EntryLo0 = (tlb_e[index].pfn_even << 6) | (tlb_e[index].c_even << 3) | (tlb_e[index].d_even << 2) | (tlb_e[index].v_even << 1) | tlb_e[index].g;
    core_EntryLo1 = (tlb_e[index].pfn_odd << 6) | (tlb_e[index].c_odd << 3) | (tlb_e[index].d_odd << 2) | (tlb_e[index].v_odd << 1) | tlb_e[index].g;
    PC++;
}

void TLBWI()
{
    uint32_t i;

    if (tlb_e[core_Index & 0x3F].v_even)
    {
        for (i = tlb_e[core_Index & 0x3F].start_even >> 12; i <= tlb_e[core_Index & 0x3F].end_even >> 12; i++)
        {
            if (!invalid_code[i] && (invalid_code[tlb_LUT_r[i] >> 12] || invalid_code[(tlb_LUT_r[i] >> 12) + 0x20000]))
                invalid_code[i] = 1;
            if (!invalid_code[i])
            {

                blocks[i]->hash = xxh64::hash((const char*)&rdram[(tlb_LUT_r[i] & 0x7FF000) / 4], 0x1000, 0);
                invalid_code[i] = 1;
            }
            else if (blocks[i])
            {
                blocks[i]->hash = 0;
            }
            tlb_LUT_r[i] = 0;
        }
        if (tlb_e[core_Index & 0x3F].d_even)
            for (i = tlb_e[core_Index & 0x3F].start_even >> 12; i <= tlb_e[core_Index & 0x3F].end_even >> 12; i++)
                tlb_LUT_w[i] = 0;
    }
    if (tlb_e[core_Index & 0x3F].v_odd)
    {
        for (i = tlb_e[core_Index & 0x3F].start_odd >> 12; i <= tlb_e[core_Index & 0x3F].end_odd >> 12; i++)
        {
            if (!invalid_code[i] && (invalid_code[tlb_LUT_r[i] >> 12] || invalid_code[(tlb_LUT_r[i] >> 12) + 0x20000]))
                invalid_code[i] = 1;
            if (!invalid_code[i])
            {
                blocks[i]->hash = xxh64::hash((const char*)&rdram[(tlb_LUT_r[i] & 0x7FF000) / 4], 0x1000, 0);
                invalid_code[i] = 1;
            }
            else if (blocks[i])
            {
                blocks[i]->hash = 0;
            }
            tlb_LUT_r[i] = 0;
        }
        if (tlb_e[core_Index & 0x3F].d_odd)
            for (i = tlb_e[core_Index & 0x3F].start_odd >> 12; i <= tlb_e[core_Index & 0x3F].end_odd >> 12; i++)
                tlb_LUT_w[i] = 0;
    }
    tlb_e[core_Index & 0x3F].g = (core_EntryLo0 & core_EntryLo1 & 1);
    tlb_e[core_Index & 0x3F].pfn_even = (core_EntryLo0 & 0x3FFFFFC0) >> 6;
    tlb_e[core_Index & 0x3F].pfn_odd = (core_EntryLo1 & 0x3FFFFFC0) >> 6;
    tlb_e[core_Index & 0x3F].c_even = (core_EntryLo0 & 0x38) >> 3;
    tlb_e[core_Index & 0x3F].c_odd = (core_EntryLo1 & 0x38) >> 3;
    tlb_e[core_Index & 0x3F].d_even = (core_EntryLo0 & 0x4) >> 2;
    tlb_e[core_Index & 0x3F].d_odd = (core_EntryLo1 & 0x4) >> 2;
    tlb_e[core_Index & 0x3F].v_even = (core_EntryLo0 & 0x2) >> 1;
    tlb_e[core_Index & 0x3F].v_odd = (core_EntryLo1 & 0x2) >> 1;
    tlb_e[core_Index & 0x3F].asid = (core_EntryHi & 0xFF);
    tlb_e[core_Index & 0x3F].vpn2 = (core_EntryHi & 0xFFFFE000) >> 13;
    // tlb_e[Index&0x3F].r = (EntryHi & 0xC000000000000000LL) >> 62;
    tlb_e[core_Index & 0x3F].mask = (core_PageMask & 0x1FFE000) >> 13;

    tlb_e[core_Index & 0x3F].start_even = tlb_e[core_Index & 0x3F].vpn2 << 13;
    tlb_e[core_Index & 0x3F].end_even = tlb_e[core_Index & 0x3F].start_even +
    (tlb_e[core_Index & 0x3F].mask << 12) + 0xFFF;
    tlb_e[core_Index & 0x3F].phys_even = tlb_e[core_Index & 0x3F].pfn_even << 12;

    if (tlb_e[core_Index & 0x3F].v_even)
    {
        if (tlb_e[core_Index & 0x3F].start_even < tlb_e[core_Index & 0x3F].end_even &&
            !(tlb_e[core_Index & 0x3F].start_even >= 0x80000000 &&
              tlb_e[core_Index & 0x3F].end_even < 0xC0000000) &&
            tlb_e[core_Index & 0x3F].phys_even < 0x20000000)
        {
            for (i = tlb_e[core_Index & 0x3F].start_even; i < tlb_e[core_Index & 0x3F].end_even; i++)
                tlb_LUT_r[i >> 12] = 0x80000000 |
                (tlb_e[core_Index & 0x3F].phys_even + (i - tlb_e[core_Index & 0x3F].start_even));
            if (tlb_e[core_Index & 0x3F].d_even)
                for (i = tlb_e[core_Index & 0x3F].start_even; i < tlb_e[core_Index & 0x3F].end_even; i++)
                    tlb_LUT_w[i >> 12] = 0x80000000 |
                    (tlb_e[core_Index & 0x3F].phys_even + (i - tlb_e[core_Index & 0x3F].start_even));
        }

        for (i = tlb_e[core_Index & 0x3F].start_even >> 12; i <= tlb_e[core_Index & 0x3F].end_even >> 12; i++)
        {
            if (blocks[i] && blocks[i]->hash)
            {
                if (blocks[i]->hash == xxh64::hash((const char*)&rdram[(tlb_LUT_r[i] & 0x7FF000) / 4], 0x1000, 0))
                    invalid_code[i] = 0;
            }
        }
    }
    tlb_e[core_Index & 0x3F].start_odd = tlb_e[core_Index & 0x3F].end_even + 1;
    tlb_e[core_Index & 0x3F].end_odd = tlb_e[core_Index & 0x3F].start_odd +
    (tlb_e[core_Index & 0x3F].mask << 12) + 0xFFF;
    tlb_e[core_Index & 0x3F].phys_odd = tlb_e[core_Index & 0x3F].pfn_odd << 12;

    if (tlb_e[core_Index & 0x3F].v_odd)
    {
        if (tlb_e[core_Index & 0x3F].start_odd < tlb_e[core_Index & 0x3F].end_odd &&
            !(tlb_e[core_Index & 0x3F].start_odd >= 0x80000000 &&
              tlb_e[core_Index & 0x3F].end_odd < 0xC0000000) &&
            tlb_e[core_Index & 0x3F].phys_odd < 0x20000000)
        {
            for (i = tlb_e[core_Index & 0x3F].start_odd; i < tlb_e[core_Index & 0x3F].end_odd; i++)
                tlb_LUT_r[i >> 12] = 0x80000000 |
                (tlb_e[core_Index & 0x3F].phys_odd + (i - tlb_e[core_Index & 0x3F].start_odd));
            if (tlb_e[core_Index & 0x3F].d_odd)
                for (i = tlb_e[core_Index & 0x3F].start_odd; i < tlb_e[core_Index & 0x3F].end_odd; i++)
                    tlb_LUT_w[i >> 12] = 0x80000000 |
                    (tlb_e[core_Index & 0x3F].phys_odd + (i - tlb_e[core_Index & 0x3F].start_odd));
        }

        for (i = tlb_e[core_Index & 0x3F].start_odd >> 12; i <= tlb_e[core_Index & 0x3F].end_odd >> 12; i++)
        {
            if (blocks[i] && blocks[i]->hash)
            {
                if (blocks[i]->hash == xxh64::hash((const char*)&rdram[(tlb_LUT_r[i] & 0x7FF000) / 4], 0x1000, 0))
                    invalid_code[i] = 0;
            }
        }
    }
    PC++;
}

void TLBWR()
{
    uint32_t i;
    update_count();
    core_Random = (core_Count / 2 % (32 - core_Wired)) + core_Wired;

    if (tlb_e[core_Random].v_even)
    {
        for (i = tlb_e[core_Random].start_even >> 12; i <= tlb_e[core_Random].end_even >> 12; i++)
        {
            if (!invalid_code[i] && (invalid_code[tlb_LUT_r[i] >> 12] || invalid_code[(tlb_LUT_r[i] >> 12) + 0x20000]))
                invalid_code[i] = 1;
            if (!invalid_code[i])
            {
                blocks[i]->hash = xxh64::hash((const char*)&rdram[(tlb_LUT_r[i] & 0x7FF000) / 4], 0x1000, 0);
                invalid_code[i] = 1;
            }
            else if (blocks[i])
            {
                blocks[i]->hash = 0;
            }
            tlb_LUT_r[i] = 0;
        }
        if (tlb_e[core_Random].d_even)
            for (i = tlb_e[core_Random].start_even >> 12; i <= tlb_e[core_Random].end_even >> 12; i++)
                tlb_LUT_w[i] = 0;
    }
    if (tlb_e[core_Random].v_odd)
    {
        for (i = tlb_e[core_Random].start_odd >> 12; i <= tlb_e[core_Random].end_odd >> 12; i++)
        {
            if (!invalid_code[i] && (invalid_code[tlb_LUT_r[i] >> 12] || invalid_code[(tlb_LUT_r[i] >> 12) + 0x20000]))
                invalid_code[i] = 1;
            if (!invalid_code[i])
            {
                blocks[i]->hash = xxh64::hash((const char*)&rdram[(tlb_LUT_r[i] & 0x7FF000) / 4], 0x1000, 0);
                invalid_code[i] = 1;
            }
            else if (blocks[i])
            {
                blocks[i]->hash = 0;
            }
            tlb_LUT_r[i] = 0;
        }
        if (tlb_e[core_Random].d_odd)
            for (i = tlb_e[core_Random].start_odd >> 12; i <= tlb_e[core_Random].end_odd >> 12; i++)
                tlb_LUT_w[i] = 0;
    }
    tlb_e[core_Random].g = (core_EntryLo0 & core_EntryLo1 & 1);
    tlb_e[core_Random].pfn_even = (core_EntryLo0 & 0x3FFFFFC0) >> 6;
    tlb_e[core_Random].pfn_odd = (core_EntryLo1 & 0x3FFFFFC0) >> 6;
    tlb_e[core_Random].c_even = (core_EntryLo0 & 0x38) >> 3;
    tlb_e[core_Random].c_odd = (core_EntryLo1 & 0x38) >> 3;
    tlb_e[core_Random].d_even = (core_EntryLo0 & 0x4) >> 2;
    tlb_e[core_Random].d_odd = (core_EntryLo1 & 0x4) >> 2;
    tlb_e[core_Random].v_even = (core_EntryLo0 & 0x2) >> 1;
    tlb_e[core_Random].v_odd = (core_EntryLo1 & 0x2) >> 1;
    tlb_e[core_Random].asid = (core_EntryHi & 0xFF);
    tlb_e[core_Random].vpn2 = (core_EntryHi & 0xFFFFE000) >> 13;
    // tlb_e[Random].r = (EntryHi & 0xC000000000000000LL) >> 62;
    tlb_e[core_Random].mask = (core_PageMask & 0x1FFE000) >> 13;

    tlb_e[core_Random].start_even = tlb_e[core_Random].vpn2 << 13;
    tlb_e[core_Random].end_even = tlb_e[core_Random].start_even +
    (tlb_e[core_Random].mask << 12) + 0xFFF;
    tlb_e[core_Random].phys_even = tlb_e[core_Random].pfn_even << 12;

    if (tlb_e[core_Random].v_even)
    {
        if (tlb_e[core_Random].start_even < tlb_e[core_Random].end_even &&
            !(tlb_e[core_Random].start_even >= 0x80000000 &&
              tlb_e[core_Random].end_even < 0xC0000000) &&
            tlb_e[core_Random].phys_even < 0x20000000)
        {
            for (i = tlb_e[core_Random].start_even; i < tlb_e[core_Random].end_even; i++)
                tlb_LUT_r[i >> 12] = 0x80000000 |
                (tlb_e[core_Random].phys_even + (i - tlb_e[core_Random].start_even));
            if (tlb_e[core_Random].d_even)
                for (i = tlb_e[core_Random].start_even; i < tlb_e[core_Random].end_even; i++)
                    tlb_LUT_w[i >> 12] = 0x80000000 |
                    (tlb_e[core_Random].phys_even + (i - tlb_e[core_Random].start_even));
        }

        for (i = tlb_e[core_Random].start_even >> 12; i <= tlb_e[core_Random].end_even >> 12; i++)
        {
            if (blocks[i] && blocks[i]->hash)
            {
                if (blocks[i]->hash == xxh64::hash((const char*)&rdram[(tlb_LUT_r[i] & 0x7FF000) / 4], 0x1000, 0))
                    invalid_code[i] = 0;
            }
        }
    }
    tlb_e[core_Random].start_odd = tlb_e[core_Random].end_even + 1;
    tlb_e[core_Random].end_odd = tlb_e[core_Random].start_odd +
    (tlb_e[core_Random].mask << 12) + 0xFFF;
    tlb_e[core_Random].phys_odd = tlb_e[core_Random].pfn_odd << 12;

    if (tlb_e[core_Random].v_odd)
    {
        if (tlb_e[core_Random].start_odd < tlb_e[core_Random].end_odd &&
            !(tlb_e[core_Random].start_odd >= 0x80000000 &&
              tlb_e[core_Random].end_odd < 0xC0000000) &&
            tlb_e[core_Random].phys_odd < 0x20000000)
        {
            for (i = tlb_e[core_Random].start_odd; i < tlb_e[core_Random].end_odd; i++)
                tlb_LUT_r[i >> 12] = 0x80000000 |
                (tlb_e[core_Random].phys_odd + (i - tlb_e[core_Random].start_odd));
            if (tlb_e[core_Random].d_odd)
                for (i = tlb_e[core_Random].start_odd; i < tlb_e[core_Random].end_odd; i++)
                    tlb_LUT_w[i >> 12] = 0x80000000 |
                    (tlb_e[core_Random].phys_odd + (i - tlb_e[core_Random].start_odd));
        }

        for (i = tlb_e[core_Random].start_odd >> 12; i <= tlb_e[core_Random].end_odd >> 12; i++)
        {
            if (blocks[i] && blocks[i]->hash)
            {
                if (blocks[i]->hash == xxh64::hash((const char*)&rdram[(tlb_LUT_r[i] & 0x7FF000) / 4], 0x1000, 0))
                    invalid_code[i] = 0;
            }
        }
    }
    PC++;
}

void TLBP()
{
    int32_t i;
    core_Index |= 0x80000000;
    for (i = 0; i < 32; i++)
    {
        if (((tlb_e[i].vpn2 & (~tlb_e[i].mask)) ==
             (((core_EntryHi & 0xFFFFE000) >> 13) & (~tlb_e[i].mask))) &&
            ((tlb_e[i].g) ||
             (tlb_e[i].asid == (core_EntryHi & 0xFF))))
        {
            core_Index = i;
            break;
        }
    }
    PC++;
}

void ERET()
{
    update_count();
    if (core_Status & 0x4)
    {
        g_core->log_error(L"erreur dans ERET");
        stop = 1;
    }
    else
    {
        core_Status &= 0xFFFFFFFD;
        jump_to(core_EPC);
    }
    llbit = 0;
    check_interrupt();
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}
