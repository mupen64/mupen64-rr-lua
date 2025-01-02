/**
 * Mupen64 - gtlb.c
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

#include "../ops.h"
#include "../r4300.h"
#include "../recomph.h"
#include "assemble.h"
#include <stdio.h>

void gentlbwi() {
  gencallinterp((uint32_t)TLBWI, 0);
  /*dst->local_addr = code_length;
  mov_m32_imm32((void *)(&PC), (uint32_t)(dst));
  mov_reg32_imm32(EAX, (uint32_t)(TLBWI));
  call_reg32(EAX);
  genupdate_system(0);*/
}

void gentlbp() {
  gencallinterp((uint32_t)TLBP, 0);
  /*dst->local_addr = code_length;
  mov_m32_imm32((void *)(&PC), (uint32_t)(dst));
  mov_reg32_imm32(EAX, (uint32_t)(TLBP));
  call_reg32(EAX);
  genupdate_system(0);*/
}

void gentlbr() {
  gencallinterp((uint32_t)TLBR, 0);
  /*dst->local_addr = code_length;
  mov_m32_imm32((void *)(&PC), (uint32_t)(dst));
  mov_reg32_imm32(EAX, (uint32_t)(TLBR));
  call_reg32(EAX);
  genupdate_system(0);*/
}

void generet() {
  gencallinterp((uint32_t)ERET, 1);
  /*dst->local_addr = code_length;
  mov_m32_imm32((void *)(&PC), (uint32_t)(dst));
  genupdate_system(0);
  mov_reg32_imm32(EAX, (uint32_t)(ERET));
  call_reg32(EAX);
  mov_reg32_imm32(EAX, (uint32_t)(jump_code));
  jmp_reg32(EAX);*/
}

void gentlbwr() { gencallinterp((uint32_t)TLBWR, 0); }
