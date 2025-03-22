/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

void compare_interrupt();
void gen_dp();
void init_interrupt();

void gen_interrupt();
void check_interrupt();

void translate_event_queue(uint32_t base);
void remove_event(int32_t type);
void add_interrupt_event_count(int32_t type, uint32_t count);
void add_interrupt_event(int32_t type, uint32_t delay);
uint32_t get_event(int32_t type);

int32_t save_eventqueue_infos(char* buf);
void load_eventqueue_infos(char* buf);

#define VI_INT 0x001
#define COMPARE_INT 0x002
#define CHECK_INT 0x004
#define SI_INT 0x008
#define PI_INT 0x010
#define SPECIAL_INT 0x020
#define AI_INT 0x040
#define SP_INT 0x080
#define DP_INT 0x100
