/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "Main.hpp"

#define S 1
#define S8 3

#define A_INIT 0x01
#define A_CONTINUE 0x00
#define A_LOOP 0x02
#define A_OUT 0x02
#define A_LEFT 0x02
#define A_RIGHT 0x00
#define A_VOL 0x04
#define A_RATE 0x00
#define A_AUX 0x08
#define A_NOAUX 0x00
#define A_MAIN 0x00
#define A_MIX 0x10

extern M64RRSpec::PluginInit* rsp;

typedef struct
{
    uint32_t type;
    uint32_t flags;

    uint32_t ucode_boot;
    uint32_t ucode_boot_size;

    uint32_t ucode;
    uint32_t ucode_size;

    uint32_t ucode_data;
    uint32_t ucode_data_size;

    uint32_t dram_stack;
    uint32_t dram_stack_size;

    uint32_t output_buff;
    uint32_t output_buff_size;

    uint32_t data_ptr;
    uint32_t data_size;

    uint32_t yield_data_ptr;
    uint32_t yield_data_size;
} OSTask_t;

void jpg_uncompress(OSTask_t *task);
void MP3();

extern uint32_t inst1, inst2;
extern uint16_t AudioInBuffer, AudioOutBuffer, AudioCount;
extern uint16_t AudioAuxA, AudioAuxC, AudioAuxE;
extern uint32_t loopval;

void decode_input_block(uint8_t *buffer, uint16_t &inPtr, int32_t *inp, uint8_t code, int32_t vscale);
void compute_and_pack_block(int32_t *inp, int16_t *book1, int16_t *book2, int32_t &l1, int32_t &l2, int16_t *&out);
