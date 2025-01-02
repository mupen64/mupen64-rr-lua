/**
 * Mupen64 - gcop0.c
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
#include "../recomp.h"
#include "../recomph.h"
#include "assemble.h"
#include <stdio.h>

// static uint32_t pMFC0 = (uint32_t)(MFC0);
void genmfc0() {
  gencallinterp((uint32_t)MFC0, 0);
  /*dst->local_addr = code_length;
  mov_m32_imm32((void *)(&PC), (uint32_t)(dst));
  call_m32((uint32_t *)(&pMFC0));
  genupdate_system(0);*/
}

// static uint32_t pMTC0 = (uint32_t)(MTC0);
void genmtc0() {
  gencallinterp((uint32_t)MTC0, 0);
  /*dst->local_addr = code_length;
  mov_m32_imm32((void *)(&PC), (uint32_t)(dst));
  call_m32((uint32_t *)(&pMTC0));
  genupdate_system(0);*/
}
