/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <r4300/r4300.h>
#include <r4300/macros.h>
#include <r4300/ops.h>
#include <r4300/interrupt.h>

void MFC0(void)
{
   switch(PC->f.r.nrd)
   {
      case 1:
        g_core->logger->error("MFC0 instruction reading un-implemented core_Random register");
        stop=1;
      default:
        rrt32 = reg_cop0[PC->f.r.nrd];
        sign_extended(core_rrt);
   }
   PC++;
}

void MTC0(void)
{
  switch(PC->f.r.nrd)
  {
    case 0:    // core_Index
      core_Index = (unsigned int) core_rrt & 0x8000003F;
      if ((core_Index & 0x3F) > 31) 
      {
        g_core->logger->error("MTC0 instruction writing core_Index register with TLB index > 31");
        stop=1;
      }
      break;
    case 1:    // core_Random
      break;
    case 2:    // core_EntryLo0
      core_EntryLo0 = (unsigned int) core_rrt & 0x3FFFFFFF;
      break;
    case 3:    // core_EntryLo1
      core_EntryLo1 = (unsigned int) core_rrt & 0x3FFFFFFF;
      break;
    case 4:    // core_Context
      core_Context = ((unsigned int) core_rrt & 0xFF800000) | (core_Context & 0x007FFFF0);
      break;
    case 5:    // core_PageMask
      core_PageMask = (unsigned int) core_rrt & 0x01FFE000;
      break;
    case 6:    // core_Wired
      core_Wired = (unsigned int) core_rrt;
      core_Random = 31;
      break;
    case 8:    // BadVAddr
      break;
    case 9:    // core_Count
      update_count();
      interupt_unsafe_state = 1;
      if (next_interrupt <= core_Count) gen_interrupt();
      interupt_unsafe_state = 0;
      debug_count += core_Count;
      translate_event_queue((unsigned int) core_rrt & 0xFFFFFFFF);
      core_Count = (unsigned int) core_rrt & 0xFFFFFFFF;
      debug_count -= core_Count;
      break;
    case 10:   // core_EntryHi
      core_EntryHi = (unsigned int) core_rrt & 0xFFFFE0FF;
      break;
    case 11:   // core_Compare
      update_count();
      remove_event(COMPARE_INT);
      add_interrupt_event_count(COMPARE_INT, (unsigned int)core_rrt);
      core_Compare = (unsigned int) core_rrt;
      core_Cause = core_Cause & 0xFFFF7FFF; //Timer interupt is clear
      break;
    case 12:   // core_Status
      if((core_rrt & 0x04000000) != (core_Status & 0x04000000))
      {
          shuffle_fpr_data(core_Status, (unsigned int) core_rrt);
          set_fpr_pointers((unsigned int) core_rrt);
      }
      core_Status = (unsigned int) core_rrt;
      PC++;
      check_interrupt();
      update_count();
      interupt_unsafe_state = 1;
      if (next_interrupt <= core_Count) gen_interrupt();
      interupt_unsafe_state = 0;
      PC--;
      break;
    case 13:   // core_Cause
      if (core_rrt!=0)
      {
         g_core->logger->error("MTC0 instruction trying to write core_Cause register with non-0 value");
         stop = 1;
      }
      else core_Cause = (unsigned int) core_rrt;
      break;
    case 14:   // core_EPC
      core_EPC = (unsigned int) core_rrt;
      break;
    case 15:  // PRevID
      break;
    case 16:  // core_Config
      core_Config = (unsigned int) core_rrt;
      break;
    case 18:  // WatchLo
      core_WatchLo = (unsigned int) core_rrt & 0xFFFFFFFF;
      break;
    case 19:  // WatchHi
      core_WatchHi = (unsigned int) core_rrt & 0xFFFFFFFF;
      break;
    case 27:  // CacheErr
      break;
    case 28:  // TagLo
      core_TagLo = (unsigned int) core_rrt & 0x0FFFFFC0;
      break;
    case 29: // TagHi
      core_TagHi =0;
      break;
    default:
      g_core->logger->error("Unknown MTC0 write: %d", PC->f.r.nrd);
      stop=1;
  }
  PC++;
}


