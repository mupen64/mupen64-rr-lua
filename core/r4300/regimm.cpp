/**
 * Mupen64 - regimm.c
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

#include "../memory/memory.h"
#include "interrupt.h"
#include "macros.h"
#include "ops.h"
#include "r4300.h"
#include <shared/services/LoggingService.h>

void BLTZ() {
  local_rs = core_irs;
  PC++;
  delay_slot = 1;
  PC->ops();
  update_count();
  delay_slot = 0;
  if (local_rs < 0 && !skip_jump)
    PC += (PC - 2)->f.i.immediate - 1;
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BLTZ_OUT() {
  local_rs = core_irs;
  jump_target = (int32_t)PC->f.i.immediate;
  PC++;
  delay_slot = 1;
  PC->ops();
  update_count();
  delay_slot = 0;
  if (!skip_jump && local_rs < 0)
    jump_to(PC->addr + ((jump_target - 1) << 2));
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BLTZ_IDLE() {
  int32_t skip;
  if (core_irs < 0) {
    update_count();
    skip = next_interrupt - core_Count;
    if (skip > 3)
      core_Count += (skip & 0xFFFFFFFC);
    else
      BLTZ();
  } else
    BLTZ();
}

void BGEZ() {
  local_rs = core_irs;
  PC++;
  delay_slot = 1;
  PC->ops();
  update_count();
  delay_slot = 0;
  if (local_rs >= 0 && !skip_jump)
    PC += (PC - 2)->f.i.immediate - 1;
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BGEZ_OUT() {
  local_rs = core_irs;
  jump_target = (int32_t)PC->f.i.immediate;
  PC++;
  delay_slot = 1;
  PC->ops();
  update_count();
  delay_slot = 0;
  if (!skip_jump && local_rs >= 0)
    jump_to(PC->addr + ((jump_target - 1) << 2));
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BGEZ_IDLE() {
  int32_t skip;
  if (core_irs >= 0) {
    update_count();
    skip = next_interrupt - core_Count;
    if (skip > 3)
      core_Count += (skip & 0xFFFFFFFC);
    else
      BGEZ();
  } else
    BGEZ();
}

void BLTZL() {
  if (core_irs < 0) {
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump)
      PC += (PC - 2)->f.i.immediate - 1;
  } else
    PC += 2;
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BLTZL_OUT() {
  if (core_irs < 0) {
    jump_target = (int32_t)PC->f.i.immediate;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump)
      jump_to(PC->addr + ((jump_target - 1) << 2));
  } else
    PC += 2;
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BLTZL_IDLE() {
  int32_t skip;
  if (core_irs < 0) {
    update_count();
    skip = next_interrupt - core_Count;
    if (skip > 3)
      core_Count += (skip & 0xFFFFFFFC);
    else
      BLTZL();
  } else
    BLTZL();
}

void BGEZL() {
  if (core_irs >= 0) {
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump)
      PC += (PC - 2)->f.i.immediate - 1;
  } else
    PC += 2;
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BGEZL_OUT() {
  if (core_irs >= 0) {
    jump_target = (int32_t)PC->f.i.immediate;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump)
      jump_to(PC->addr + ((jump_target - 1) << 2));
  } else
    PC += 2;
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BGEZL_IDLE() {
  int32_t skip;
  if (core_irs >= 0) {
    update_count();
    skip = next_interrupt - core_Count;
    if (skip > 3)
      core_Count += (skip & 0xFFFFFFFC);
    else
      BGEZL();
  } else
    BGEZL();
}

void BLTZAL() {
  local_rs = core_irs;
  reg[31] = PC->addr + 8;
  if ((&core_irs) != (reg + 31)) {
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (local_rs < 0 && !skip_jump)
      PC += (PC - 2)->f.i.immediate - 1;
  } else
    g_core_logger->error("erreur dans bltzal");
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BLTZAL_OUT() {
  local_rs = core_irs;
  reg[31] = PC->addr + 8;
  if ((&core_irs) != (reg + 31)) {
    jump_target = (int32_t)PC->f.i.immediate;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump && local_rs < 0)
      jump_to(PC->addr + ((jump_target - 1) << 2));
  } else
    g_core_logger->error("erreur dans bltzal");
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BLTZAL_IDLE() {
  int32_t skip;
  if (core_irs < 0) {
    update_count();
    skip = next_interrupt - core_Count;
    if (skip > 3)
      core_Count += (skip & 0xFFFFFFFC);
    else
      BLTZAL();
  } else
    BLTZAL();
}

void BGEZAL() {
  local_rs = core_irs;
  reg[31] = PC->addr + 8;
  if ((&core_irs) != (reg + 31)) {
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (local_rs >= 0 && !skip_jump)
      PC += (PC - 2)->f.i.immediate - 1;
  } else
    g_core_logger->info("erreur dans bgezal");
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BGEZAL_OUT() {
  local_rs = core_irs;
  reg[31] = PC->addr + 8;
  if ((&core_irs) != (reg + 31)) {
    jump_target = (int32_t)PC->f.i.immediate;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump && local_rs >= 0)
      jump_to(PC->addr + ((jump_target - 1) << 2));
  } else
    g_core_logger->info("erreur dans bgezal");
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BGEZAL_IDLE() {
  int32_t skip;
  if (core_irs >= 0) {
    update_count();
    skip = next_interrupt - core_Count;
    if (skip > 3)
      core_Count += (skip & 0xFFFFFFFC);
    else
      BGEZAL();
  } else
    BGEZAL();
}

void BLTZALL() {
  local_rs = core_irs;
  reg[31] = PC->addr + 8;
  if ((&core_irs) != (reg + 31)) {
    if (local_rs < 0) {
      PC++;
      delay_slot = 1;
      PC->ops();
      update_count();
      delay_slot = 0;
      if (!skip_jump)
        PC += (PC - 2)->f.i.immediate - 1;
    } else
      PC += 2;
  } else
    g_core_logger->info("erreur dans bltzall");
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BLTZALL_OUT() {
  local_rs = core_irs;
  reg[31] = PC->addr + 8;
  if ((&core_irs) != (reg + 31)) {
    if (local_rs < 0) {
      jump_target = (int32_t)PC->f.i.immediate;
      PC++;
      delay_slot = 1;
      PC->ops();
      update_count();
      delay_slot = 0;
      if (!skip_jump)
        jump_to(PC->addr + ((jump_target - 1) << 2));
    } else
      PC += 2;
  } else
    g_core_logger->info("erreur dans bltzall");
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BLTZALL_IDLE() {
  int32_t skip;
  if (core_irs < 0) {
    update_count();
    skip = next_interrupt - core_Count;
    if (skip > 3)
      core_Count += (skip & 0xFFFFFFFC);
    else
      BLTZALL();
  } else
    BLTZALL();
}

void BGEZALL() {
  local_rs = core_irs;
  reg[31] = PC->addr + 8;
  if ((&core_irs) != (reg + 31)) {
    if (local_rs >= 0) {
      PC++;
      delay_slot = 1;
      PC->ops();
      update_count();
      delay_slot = 0;
      if (!skip_jump)
        PC += (PC - 2)->f.i.immediate - 1;
    } else
      PC += 2;
  } else
    g_core_logger->info("erreur dans bgezall");
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BGEZALL_OUT() {
  local_rs = core_irs;
  reg[31] = PC->addr + 8;
  if ((&core_irs) != (reg + 31)) {
    if (local_rs >= 0) {
      jump_target = (int32_t)PC->f.i.immediate;
      PC++;
      delay_slot = 1;
      PC->ops();
      update_count();
      delay_slot = 0;
      if (!skip_jump)
        jump_to(PC->addr + ((jump_target - 1) << 2));
    } else
      PC += 2;
  } else
    g_core_logger->info("erreur dans bgezall");
  last_addr = PC->addr;
  if (next_interrupt <= core_Count)
    gen_interrupt();
}

void BGEZALL_IDLE() {
  int32_t skip;
  if (core_irs >= 0) {
    update_count();
    skip = next_interrupt - core_Count;
    if (skip > 3)
      core_Count += (skip & 0xFFFFFFFC);
    else
      BGEZALL();
  } else
    BGEZALL();
}
