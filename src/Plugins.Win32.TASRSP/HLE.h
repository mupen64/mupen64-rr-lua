/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "core_plugin.h"

#define S 1
#define S8 3

/*
 * Audio flags
 */

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

extern core_rsp_info rsp;

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
extern uint32_t loopval; // Value set by A_SETLOOP : Possible conflict with SETVOLUME???
