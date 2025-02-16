/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

typedef struct _tlb
{
    short mask;
    int vpn2;
    char g;
    unsigned char asid;
    int pfn_even;
    char c_even;
    char d_even;
    char v_even;
    int pfn_odd;
    char c_odd;
    char d_odd;
    char v_odd;
    char r;
    //int check_parity_mask;
   
    unsigned int start_even;
    unsigned int end_even;
    unsigned int phys_even;
    unsigned int start_odd;
    unsigned int end_odd;
    unsigned int phys_odd;
} tlb;

extern unsigned int tlb_LUT_r[0x100000];
extern unsigned int tlb_LUT_w[0x100000];
void tlb_unmap(tlb *entry);
void tlb_map(tlb *entry);
unsigned int virtual_to_physical_address(unsigned int addresse, int w);
int probe_nop(unsigned int address);
