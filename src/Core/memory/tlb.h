/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

typedef struct _tlb {
    int16_t mask;
    int32_t vpn2;
    char g;
    unsigned char asid;
    int32_t pfn_even;
    char c_even;
    char d_even;
    char v_even;
    int32_t pfn_odd;
    char c_odd;
    char d_odd;
    char v_odd;
    char r;
    // int32_t check_parity_mask;

    uint32_t start_even;
    uint32_t end_even;
    uint32_t phys_even;
    uint32_t start_odd;
    uint32_t end_odd;
    uint32_t phys_odd;
} tlb;

extern uint32_t tlb_LUT_r[0x100000];
extern uint32_t tlb_LUT_w[0x100000];
uint32_t virtual_to_physical_address(uint32_t addresse, int32_t w);
int32_t probe_nop(uint32_t address);
