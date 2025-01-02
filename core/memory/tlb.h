/**
 * Mupen64 - tlb.h
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

#ifndef TLB_H
#define TLB_H

#include <cstdint>

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

// uint32_t& get_tlb_LUT_r(int32_t index)
//{
//	return ::tlb_LUT_r[index];
// }
// uint32_t& get_tlb_LUT_w(int32_t index)
//{
//	return ::tlb_LUT_w[index];
// }
#endif
