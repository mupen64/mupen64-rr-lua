/**
 * Mupen64 - rjump.c
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

#include <cstdlib>

#include "../macros.h"
#include "../ops.h"
#include "../r4300.h"
#include "../recomp.h"
#include "../recomph.h"
#include <csetjmp>
#include <shared/Messenger.h>
#include <shared/services/LoggingService.h>

// NOTE: dynarec isn't compatible with the game debugger

void dyna_jump() {
  if (PC->reg_cache_infos.need_map)
    *return_address = (uint32_t)(PC->reg_cache_infos.jump_wrapper);
  else
    *return_address = (uint32_t)(actual->code + PC->local_addr);
}

jmp_buf g_jmp_state;

void dyna_start(void (*code)()) {
  // code() ‚Ì‚Ç‚±‚©‚Å stop ‚ª true ‚É‚È‚Á‚½ŽžAdyna_stop()
  // ‚ªŒÄ‚Î‚êAlongjmp() ‚Å setjmp()
  // ‚µ‚½‚Æ‚±‚ë‚É–ß‚é
  // –ß‚Á‚Ä‚«‚½ setjmp() ‚Í 1 ‚ð•Ô‚·‚Ì‚ÅAdyna_start()
  // I—¹
  // ƒŒƒWƒXƒ^ ebx, esi, edi, ebp ‚Ì•Û‘¶‚Æ•œŒ³‚ª•K—v‚¾‚ªAsetjmp()
  // ‚ª‚â‚Á‚Ä‚­‚ê‚é
  core_executing = true;
  Messenger::broadcast(Messenger::Message::CoreExecutingChanged,
                       core_executing);
  g_core_logger->info(L"core_executing: {}", (bool)core_executing);
  if (setjmp(g_jmp_state) == 0) {
    code();
  }
}

void dyna_stop() {
  longjmp(g_jmp_state, 1); // goto dyna_start()
}
