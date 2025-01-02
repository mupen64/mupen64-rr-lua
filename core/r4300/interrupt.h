/**
 * Mupen64 - interrupt.h
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

#pragma once

#include <cstdint>

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
