/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "tlb.h"
#include "memory.h"
#include <zlib.h>
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

uLong ZEXPORT adler32(uLong adler, const Bytef* buf, uInt len);


void TLBR(void)
{
    int index;
    index = core_Index & 0x1F;
    core_PageMask = tlb_e[index].mask << 13;
    core_EntryHi = ((tlb_e[index].vpn2 << 13) | tlb_e[index].asid);
    core_EntryLo0 = (tlb_e[index].pfn_even << 6) | (tlb_e[index].c_even << 3) | (tlb_e[index].d_even << 2) | (tlb_e[index].v_even << 1) | tlb_e[index].g;
    core_EntryLo1 = (tlb_e[index].pfn_odd << 6) | (tlb_e[index].c_odd << 3) | (tlb_e[index].d_odd << 2) | (tlb_e[index].v_odd << 1) | tlb_e[index].g;
    PC++;
}


static void TLBWrite(unsigned int idx)
{
    unsigned int i;

    if (tlb_e[idx].v_even)
    {
        for (i = tlb_e[idx].start_even >> 12; i <= tlb_e[idx].end_even >> 12; i++)
        {
            if (!invalid_code[i] && (invalid_code[tlb_LUT_r[i] >> 12] || invalid_code[(tlb_LUT_r[i] >> 12) + 0x20000]))
                invalid_code[i] = 1;
            if (!invalid_code[i])
            {
                blocks[i]->adler32 = adler32(0, (const unsigned char*)&rdram[(tlb_LUT_r[i] & 0x7FF000) / 4], 0x1000);

                invalid_code[i] = 1;
            }
            else if (blocks[i])
            {
                blocks[i]->adler32 = 0;
            }
        }
    }
    if (tlb_e[idx].v_odd)
    {
        for (i = tlb_e[idx].start_odd >> 12; i <= tlb_e[idx].end_odd >> 12; i++)
        {
            if (!invalid_code[i] && (invalid_code[tlb_LUT_r[i] >> 12] || invalid_code[(tlb_LUT_r[i] >> 12) + 0x20000]))
                invalid_code[i] = 1;
            if (!invalid_code[i])
            {

                blocks[i]->adler32 = adler32(0, (const unsigned char*)&rdram[(tlb_LUT_r[i] & 0x7FF000) / 4], 0x1000);

                invalid_code[i] = 1;
            }
            else if (blocks[i])
            {
                blocks[i]->adler32 = 0;
            }
        }
    }

    tlb_unmap(&tlb_e[idx]);

    tlb_e[idx].g = (core_EntryLo0 & core_EntryLo1 & 1);
    tlb_e[idx].pfn_even = (core_EntryLo0 & 0x3FFFFFC0) >> 6;
    tlb_e[idx].pfn_odd = (core_EntryLo1 & 0x3FFFFFC0) >> 6;
    tlb_e[idx].c_even = (core_EntryLo0 & 0x38) >> 3;
    tlb_e[idx].c_odd = (core_EntryLo1 & 0x38) >> 3;
    tlb_e[idx].d_even = (core_EntryLo0 & 0x4) >> 2;
    tlb_e[idx].d_odd = (core_EntryLo1 & 0x4) >> 2;
    tlb_e[idx].v_even = (core_EntryLo0 & 0x2) >> 1;
    tlb_e[idx].v_odd = (core_EntryLo1 & 0x2) >> 1;
    tlb_e[idx].asid = (core_EntryHi & 0xFF);
    tlb_e[idx].vpn2 = (core_EntryHi & 0xFFFFE000) >> 13;
    // tlb_e[idx].r = (core_EntryHi & 0xC000000000000000LL) >> 62;
    tlb_e[idx].mask = (core_PageMask & 0x1FFE000) >> 13;

    tlb_e[idx].start_even = tlb_e[idx].vpn2 << 13;
    tlb_e[idx].end_even = tlb_e[idx].start_even +
    (tlb_e[idx].mask << 12) + 0xFFF;
    tlb_e[idx].phys_even = tlb_e[idx].pfn_even << 12;


    tlb_e[idx].start_odd = tlb_e[idx].end_even + 1;
    tlb_e[idx].end_odd = tlb_e[idx].start_odd +
    (tlb_e[idx].mask << 12) + 0xFFF;
    tlb_e[idx].phys_odd = tlb_e[idx].pfn_odd << 12;

    tlb_map(&tlb_e[idx]);

    if (tlb_e[idx].v_even)
    {
        for (i = tlb_e[idx].start_even >> 12; i <= tlb_e[idx].end_even >> 12; i++)
        {
            if (blocks[i] && blocks[i]->adler32)
            {
                if (blocks[i]->adler32 == adler32(0, (const unsigned char*)&rdram[(tlb_LUT_r[i] & 0x7FF000) / 4], 0x1000))
                    invalid_code[i] = 0;
            }
        }
    }

    if (tlb_e[idx].v_odd)
    {
        for (i = tlb_e[idx].start_odd >> 12; i <= tlb_e[idx].end_odd >> 12; i++)
        {
            if (blocks[i] && blocks[i]->adler32)
            {
                if (blocks[i]->adler32 == adler32(0, (const unsigned char*)&rdram[(tlb_LUT_r[i] & 0x7FF000) / 4], 0x1000))
                    invalid_code[i] = 0;
            }
        }
    }
}

void TLBWI(void)
{
    TLBWrite(core_Index & 0x3F);
    PC++;
}

void TLBWR(void)
{
    update_count();
    core_Random = (core_Count / 2 % (32 - core_Wired)) + core_Wired;
    TLBWrite(core_Random);
    PC++;
}

void TLBP(void)
{
    int i;
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

void ERET(void)
{
    update_count();
    if (core_Status & 0x4)
    {
        g_core->logger->error("error in ERET");
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


void tlb_unmap(tlb* entry)
{
    unsigned int i;

    if (entry->v_even)
    {
        for (i = entry->start_even; i < entry->end_even; i += 0x1000)
            tlb_LUT_r[i >> 12] = 0;
        if (entry->d_even)
            for (i = entry->start_even; i < entry->end_even; i += 0x1000)
                tlb_LUT_w[i >> 12] = 0;
    }

    if (entry->v_odd)
    {
        for (i = entry->start_odd; i < entry->end_odd; i += 0x1000)
            tlb_LUT_r[i >> 12] = 0;
        if (entry->d_odd)
            for (i = entry->start_odd; i < entry->end_odd; i += 0x1000)
                tlb_LUT_w[i >> 12] = 0;
    }
}

void tlb_map(tlb* entry)
{
    unsigned int i;

    if (entry->v_even)
    {
        if (entry->start_even < entry->end_even &&
            !(entry->start_even >= 0x80000000 && entry->end_even < 0xC0000000) &&
            entry->phys_even < 0x20000000)
        {
            for (i = entry->start_even; i < entry->end_even; i += 0x1000)
                tlb_LUT_r[i >> 12] = 0x80000000 | (entry->phys_even + (i - entry->start_even) + 0xFFF);
            if (entry->d_even)
                for (i = entry->start_even; i < entry->end_even; i += 0x1000)
                    tlb_LUT_w[i >> 12] = 0x80000000 | (entry->phys_even + (i - entry->start_even) + 0xFFF);
        }
    }

    if (entry->v_odd)
    {
        if (entry->start_odd < entry->end_odd &&
            !(entry->start_odd >= 0x80000000 && entry->end_odd < 0xC0000000) &&
            entry->phys_odd < 0x20000000)
        {
            for (i = entry->start_odd; i < entry->end_odd; i += 0x1000)
                tlb_LUT_r[i >> 12] = 0x80000000 | (entry->phys_odd + (i - entry->start_odd) + 0xFFF);
            if (entry->d_odd)
                for (i = entry->start_odd; i < entry->end_odd; i += 0x1000)
                    tlb_LUT_w[i >> 12] = 0x80000000 | (entry->phys_odd + (i - entry->start_odd) + 0xFFF);
        }
    }
}


unsigned int virtual_to_physical_address(unsigned int addresse, int w)
{
    if (addresse >= 0x7f000000 && addresse < 0x80000000 && isGoldeneyeRom)
    {
        /**************************************************
         GoldenEye 007 hack allows for use of TLB.
         Recoded by okaygo to support all US, J, and E ROMS.
        **************************************************/
        switch (ROM_HEADER.Country_code & 0xFF)
        {
        case 0x45:
            // U
            return 0xb0034b30 + (addresse & 0xFFFFFF);
            break;
        case 0x4A:
            // J
            return 0xb0034b70 + (addresse & 0xFFFFFF);
            break;
        case 0x50:
            // E
            return 0xb00329f0 + (addresse & 0xFFFFFF);
            break;
        default:
            // UNKNOWN COUNTRY CODE FOR GOLDENEYE USING AMERICAN VERSION HACK
            return 0xb0034b30 + (addresse & 0xFFFFFF);
            break;
        }
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
    // printf("tlb exception !!! @ %x, %x, add:%x\n", addresse, w, interp_addr);
    // getchar();
    TLB_refill_exception(addresse, w);
    // return 0x80000000;
    return 0x00000000;
}

int probe_nop(unsigned int address)
{
    return *fast_mem_access(address) == 0;
}
