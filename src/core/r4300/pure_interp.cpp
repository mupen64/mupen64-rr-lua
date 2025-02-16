/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <r4300/fpu.h>
#include <Core.h>
#include <include/core_api.h>
#include <memory/memory.h>
#include <memory/tlb.h>
#include <r4300/exception.h>
#include <r4300/interrupt.h>
#include <r4300/macros.h>
#include <r4300/r4300.h>

uint32_t interp_addr;
uint32_t vr_op;
static int32_t skip;

#define check_r0_rd() { if (PC->f.r.rd == reg) { interp_addr+=4; return; } }
#define check_r0_rt() { if (PC->f.r.rt == reg) { interp_addr+=4; return; } }
#define check_r0_irt() { if (PC->f.i.rt == reg) { interp_addr+=4; return; } }

void prefetch();

extern void (*interp_ops[])(void);

extern uint32_t next_vi;

static void NI()
{
    g_core->logger->info("NI:{:#06x}", (uint32_t)vr_op);
    stop = 1;
}

static void SLL(void)
{
   check_r0_rd();
   rrd32 = (unsigned int)(rrt32) << core_rsa;
   sign_extended(core_rrd);
   interp_addr+=4;
}

static void SRL(void)
{
   check_r0_rd();
   rrd32 = (unsigned int)rrt32 >> core_rsa;
   sign_extended(core_rrd);
   interp_addr+=4;
}

static void SRA(void)
{
   check_r0_rd();
   rrd32 = (signed int)rrt32 >> core_rsa;
   sign_extended(core_rrd);
   interp_addr+=4;
}

static void SLLV(void)
{
   check_r0_rd();
   rrd32 = (unsigned int)(rrt32) << (rrs32&0x1F);
   sign_extended(core_rrd);
   interp_addr+=4;
}

static void SRLV(void)
{
   check_r0_rd();
   rrd32 = (unsigned int)rrt32 >> (rrs32 & 0x1F);
   sign_extended(core_rrd);
   interp_addr+=4;
}

static void SRAV(void)
{
   check_r0_rd();
   rrd32 = (signed int)rrt32 >> (rrs32 & 0x1F);
   sign_extended(core_rrd);
   interp_addr+=4;
}

static void JR(void)
{
   local_rs32 = irs32;
   interp_addr+=4;
   delay_slot=1;
   prefetch();
   interp_ops[((vr_op >> 26) & 0x3F)]();
   update_count();
   delay_slot=0;
   interp_addr = local_rs32;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void JALR(void)
{
   unsigned long long *dest = (unsigned long long *) PC->f.r.rd;
   local_rs32 = rrs32;
   interp_addr+=4;
   delay_slot=1;
   prefetch();
   interp_ops[((vr_op >> 26) & 0x3F)]();
   update_count();
   delay_slot=0;
   if (!skip_jump)
     {
    *dest = interp_addr;
    sign_extended(*dest);
    
    interp_addr = local_rs32;
     }
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void SYSCALL(void)
{
   core_Cause = 8 << 2;
   exception_general();
}

static void SYNC(void)
{
   interp_addr+=4;
}

static void MFHI(void)
{
   check_r0_rd();
   core_rrd = hi;
   interp_addr+=4;
}

static void MTHI(void)
{
   hi = core_rrs;
   interp_addr+=4;
}

static void MFLO(void)
{
   check_r0_rd();
   core_rrd = lo;
   interp_addr+=4;
}

static void MTLO(void)
{
   lo = core_rrs;
   interp_addr+=4;
}

static void DSLLV(void)
{
   check_r0_rd();
   core_rrd = core_rrt << (rrs32&0x3F);
   interp_addr+=4;
}

static void DSRLV(void)
{
   check_r0_rd();
   core_rrd = (unsigned long long)core_rrt >> (rrs32 & 0x3F);
   interp_addr+=4;
}

static void DSRAV(void)
{
   check_r0_rd();
   core_rrd = (long long)core_rrt >> (rrs32 & 0x3F);
   interp_addr+=4;
}

static void MULT(void)
{
   long long int temp;
   temp = core_rrs * core_rrt;
   hi = temp >> 32;
   lo = temp;
   sign_extended(lo);
   interp_addr+=4;
}

static void MULTU(void)
{
   unsigned long long int temp;
   temp = (unsigned int)core_rrs * (unsigned long long)((unsigned int)core_rrt);
   hi = (long long)temp >> 32;
   lo = temp;
   sign_extended(lo);
   interp_addr+=4;
}

static void DIV(void)
{
   if (rrt32)
   {
     lo = rrs32 / rrt32;
     hi = rrs32 % rrt32;
     sign_extended(lo);
     sign_extended(hi);
   }
   else g_core->logger->error("DIV: divide by 0");
   interp_addr+=4;
}

static void DIVU(void)
{
   if (rrt32)
   {
     lo = (unsigned int)rrs32 / (unsigned int)rrt32;
     hi = (unsigned int)rrs32 % (unsigned int)rrt32;
     sign_extended(lo);
     sign_extended(hi);
   }
   else g_core->logger->error("DIVU: divide by 0");
   interp_addr+=4;
}

static void DMULT(void)
{
   unsigned long long int op1, op2, op3, op4;
   unsigned long long int result1, result2, result3, result4;
   unsigned long long int temp1, temp2, temp3, temp4;
   int sign = 0;
   
   if (core_rrs < 0)
     {
    op2 = -core_rrs;
    sign = 1 - sign;
     }
   else op2 = core_rrs;
   if (core_rrt < 0)
     {
    op4 = -core_rrt;
    sign = 1 - sign;
     }
   else op4 = core_rrt;
   
   op1 = op2 & 0xFFFFFFFF;
   op2 = (op2 >> 32) & 0xFFFFFFFF;
   op3 = op4 & 0xFFFFFFFF;
   op4 = (op4 >> 32) & 0xFFFFFFFF;
   
   temp1 = op1 * op3;
   temp2 = (temp1 >> 32) + op1 * op4;
   temp3 = op2 * op3;
   temp4 = (temp3 >> 32) + op2 * op4;
   
   result1 = temp1 & 0xFFFFFFFF;
   result2 = temp2 + (temp3 & 0xFFFFFFFF);
   result3 = (result2 >> 32) + temp4;
   result4 = (result3 >> 32);
   
   lo = result1 | (result2 << 32);
   hi = (result3 & 0xFFFFFFFF) | (result4 << 32);
   if (sign)
     {
    hi = ~hi;
    if (!lo) hi++;
    else lo = ~lo + 1;
     }
   interp_addr+=4;
}

static void DMULTU(void)
{
   unsigned long long int op1, op2, op3, op4;
   unsigned long long int result1, result2, result3, result4;
   unsigned long long int temp1, temp2, temp3, temp4;
   
   op1 = core_rrs & 0xFFFFFFFF;
   op2 = (core_rrs >> 32) & 0xFFFFFFFF;
   op3 = core_rrt & 0xFFFFFFFF;
   op4 = (core_rrt >> 32) & 0xFFFFFFFF;
   
   temp1 = op1 * op3;
   temp2 = (temp1 >> 32) + op1 * op4;
   temp3 = op2 * op3;
   temp4 = (temp3 >> 32) + op2 * op4;
   
   result1 = temp1 & 0xFFFFFFFF;
   result2 = temp2 + (temp3 & 0xFFFFFFFF);
   result3 = (result2 >> 32) + temp4;
   result4 = (result3 >> 32);
   
   lo = result1 | (result2 << 32);
   hi = (result3 & 0xFFFFFFFF) | (result4 << 32);
   
   interp_addr+=4;
}

static void DDIV(void)
{
   if (core_rrt)
   {
     lo = (long long int)core_rrs / (long long int)core_rrt;
     hi = (long long int)core_rrs % (long long int)core_rrt;
   }
   else g_core->logger->error("DDIV: divide by 0");
   interp_addr+=4;
}

static void DDIVU(void)
{
   if (core_rrt)
   {
     lo = (unsigned long long int)core_rrs / (unsigned long long int)core_rrt;
     hi = (unsigned long long int)core_rrs % (unsigned long long int)core_rrt;
   }
   else g_core->logger->error("DDIVU: divide by 0");
   interp_addr+=4;
}

static void ADD(void)
{
   check_r0_rd();
   rrd32 = rrs32 + rrt32;
   sign_extended(core_rrd);
   interp_addr+=4;
}

static void ADDU(void)
{
   check_r0_rd();
   rrd32 = rrs32 + rrt32;
   sign_extended(core_rrd);
   interp_addr+=4;
}

static void SUB(void)
{
   check_r0_rd();
   rrd32 = rrs32 - rrt32;
   sign_extended(core_rrd);
   interp_addr+=4;
}

static void SUBU(void)
{
   check_r0_rd();
   rrd32 = rrs32 - rrt32;
   sign_extended(core_rrd);
   interp_addr+=4;
}

static void AND(void)
{
   check_r0_rd();
   core_rrd = core_rrs & core_rrt;
   interp_addr+=4;
}

static void OR(void)
{
   check_r0_rd();
   core_rrd = core_rrs | core_rrt;
   interp_addr+=4;
}

static void XOR(void)
{
   check_r0_rd();
   core_rrd = core_rrs ^ core_rrt;
   interp_addr+=4;
}

static void NOR(void)
{
   check_r0_rd();
   core_rrd = ~(core_rrs | core_rrt);
   interp_addr+=4;
}

static void SLT(void)
{
   check_r0_rd();
   if (core_rrs < core_rrt) core_rrd = 1;
   else core_rrd = 0;
   interp_addr+=4;
}

static void SLTU(void)
{
   check_r0_rd();
   if ((unsigned long long)core_rrs < (unsigned long long)core_rrt)
     core_rrd = 1;
   else core_rrd = 0;
   interp_addr+=4;
}

static void DADD(void)
{
   check_r0_rd();
   core_rrd = core_rrs + core_rrt;
   interp_addr+=4;
}

static void DADDU(void)
{
   check_r0_rd();
   core_rrd = core_rrs + core_rrt;
   interp_addr+=4;
}

static void DSUB(void)
{
   check_r0_rd();
   core_rrd = core_rrs - core_rrt;
   interp_addr+=4;
}

static void DSUBU(void)
{
   check_r0_rd();
   core_rrd = core_rrs - core_rrt;
   interp_addr+=4;
}

static void TEQ(void)
{
   if (core_rrs == core_rrt)
   {
     g_core->logger->error("trap exception in TEQ");
     stop=1;
   }
   interp_addr+=4;
}

static void DSLL(void)
{
   check_r0_rd();
   core_rrd = core_rrt << core_rsa;
   interp_addr+=4;
}

static void DSRL(void)
{
   check_r0_rd();
   core_rrd = (unsigned long long)core_rrt >> core_rsa;
   interp_addr+=4;
}

static void DSRA(void)
{
   check_r0_rd();
   core_rrd = core_rrt >> core_rsa;
   interp_addr+=4;
}

static void DSLL32(void)
{
   check_r0_rd();
   core_rrd = core_rrt << (32+core_rsa);
   interp_addr+=4;
}

static void DSRL32(void)
{
   check_r0_rd();
   core_rrd = (unsigned long long int)core_rrt >> (32+core_rsa);
   interp_addr+=4;
}

static void DSRA32(void)
{
   check_r0_rd();
   core_rrd = (signed long long int)core_rrt >> (32+core_rsa);
   interp_addr+=4;
}

static void (*interp_special[64])(void) =
{
   SLL , NI   , SRL , SRA , SLLV   , NI    , SRLV  , SRAV  ,
   JR  , JALR , NI  , NI  , SYSCALL, NI    , NI    , SYNC  ,
   MFHI, MTHI , MFLO, MTLO, DSLLV  , NI    , DSRLV , DSRAV ,
   MULT, MULTU, DIV , DIVU, DMULT  , DMULTU, DDIV  , DDIVU ,
   ADD , ADDU , SUB , SUBU, AND    , OR    , XOR   , NOR   ,
   NI  , NI   , SLT , SLTU, DADD   , DADDU , DSUB  , DSUBU ,
   NI  , NI   , NI  , NI  , TEQ    , NI    , NI    , NI    ,
   DSLL, NI   , DSRL, DSRA, DSLL32 , NI    , DSRL32, DSRA32
};

static void BLTZ(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if (local_rs < 0)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   interp_addr+=4;
   delay_slot=1;
   prefetch();
   interp_ops[((vr_op >> 26) & 0x3F)]();
   update_count();
   delay_slot=0;
   if (local_rs < 0)
     interp_addr += (local_immediate-1)*4;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BGEZ(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if (local_rs >= 0)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   interp_addr+=4;
   delay_slot=1;
   prefetch();
   interp_ops[((vr_op >> 26) & 0x3F)]();
   update_count();
   delay_slot=0;
   if (local_rs >= 0)
     interp_addr += (local_immediate-1)*4;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BLTZL(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if (core_irs < 0)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   if (core_irs < 0)
     {
    interp_addr+=4;
    delay_slot=1;
    prefetch();
    interp_ops[((vr_op >> 26) & 0x3F)]();
    update_count();
    delay_slot=0;
    interp_addr += (local_immediate-1)*4;
     }
   else interp_addr+=8;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BGEZL(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if (core_irs >= 0)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   if (core_irs >= 0)
     {
    interp_addr+=4;
    delay_slot=1;
    prefetch();
    interp_ops[((vr_op >> 26) & 0x3F)]();
    update_count();
    delay_slot=0;
    interp_addr += (local_immediate-1)*4;
     }
   else interp_addr+=8;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BLTZAL(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   reg[31]=interp_addr+8;
   if((&core_irs)!=(reg+31))
     {
    if ((interp_addr + (local_immediate+1)*4) == interp_addr)
      if (local_rs < 0)
        {
           if (probe_nop(interp_addr+4))
         {
            update_count();
            skip = next_interrupt - core_Count;
            if (skip > 3) 
              {
             core_Count += (skip & 0xFFFFFFFC);
             return;
              }
         }
        }
    interp_addr+=4;
    delay_slot=1;
    prefetch();
    interp_ops[((vr_op >> 26) & 0x3F)]();
    update_count();
    delay_slot=0;
    if(local_rs < 0)
      interp_addr += (local_immediate-1)*4;
     }
   else g_core->logger->error("error in BLTZAL");
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BGEZAL(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   reg[31]=interp_addr+8;
   if((&core_irs)!=(reg+31))
     {
    if ((interp_addr + (local_immediate+1)*4) == interp_addr)
      if (local_rs >= 0)
        {
           if (probe_nop(interp_addr+4))
         {
            update_count();
            skip = next_interrupt - core_Count;
            if (skip > 3) 
              {
             core_Count += (skip & 0xFFFFFFFC);
             return;
              }
         }
        }
    interp_addr+=4;
    delay_slot=1;
    prefetch();
    interp_ops[((vr_op >> 26) & 0x3F)]();
    update_count();
    delay_slot=0;
    if(local_rs >= 0)
      interp_addr += (local_immediate-1)*4;
     }
   else g_core->logger->error("error in BGEZAL");
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BLTZALL(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   reg[31]=interp_addr+8;
   if((&core_irs)!=(reg+31))
     {
    if ((interp_addr + (local_immediate+1)*4) == interp_addr)
      if (local_rs < 0)
        {
           if (probe_nop(interp_addr+4))
         {
            update_count();
            skip = next_interrupt - core_Count;
            if (skip > 3) 
              {
             core_Count += (skip & 0xFFFFFFFC);
             return;
              }
         }
        }
    if (local_rs < 0)
      {
         interp_addr+=4;
         delay_slot=1;
         prefetch();
         interp_ops[((vr_op >> 26) & 0x3F)]();
         update_count();
         delay_slot=0;
         interp_addr += (local_immediate-1)*4;
      }
    else interp_addr+=8;
     }
   else g_core->logger->error("error in BLTZALL");
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BGEZALL(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   reg[31]=interp_addr+8;
   if((&core_irs)!=(reg+31))
     {
    if ((interp_addr + (local_immediate+1)*4) == interp_addr)
      if (local_rs >= 0)
        {
           if (probe_nop(interp_addr+4))
         {
            update_count();
            skip = next_interrupt - core_Count;
            if (skip > 3) 
              {
             core_Count += (skip & 0xFFFFFFFC);
             return;
              }
         }
        }
    if (local_rs >= 0)
      {
         interp_addr+=4;
         delay_slot=1;
         prefetch();
         interp_ops[((vr_op >> 26) & 0x3F)]();
         update_count();
         delay_slot=0;
         interp_addr += (local_immediate-1)*4;
      }
    else interp_addr+=8;
     }
   else g_core->logger->error("error in BGEZALL");
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void (*interp_regimm[32])(void) =
{
   BLTZ  , BGEZ  , BLTZL  , BGEZL  , NI, NI, NI, NI,
   NI    , NI    , NI     , NI     , NI, NI, NI, NI,
   BLTZAL, BGEZAL, BLTZALL, BGEZALL, NI, NI, NI, NI,
   NI    , NI    , NI     , NI     , NI, NI, NI, NI
};

static void TLBR(void)
{
   int index;
   index = core_Index & 0x1F;
   core_PageMask = tlb_e[index].mask << 13;
   core_EntryHi = ((tlb_e[index].vpn2 << 13) | tlb_e[index].asid);
   core_EntryLo0 = (tlb_e[index].pfn_even << 6) | (tlb_e[index].c_even << 3)
     | (tlb_e[index].d_even << 2) | (tlb_e[index].v_even << 1)
       | tlb_e[index].g;
   core_EntryLo1 = (tlb_e[index].pfn_odd << 6) | (tlb_e[index].c_odd << 3)
     | (tlb_e[index].d_odd << 2) | (tlb_e[index].v_odd << 1)
       | tlb_e[index].g;
   interp_addr+=4;
}

static void TLBWrite(unsigned int idx)
{   
   tlb_unmap(&tlb_e[idx]);

   tlb_e[idx].g = (core_EntryLo0 & core_EntryLo1 & 1);
   tlb_e[idx].pfn_even = (core_EntryLo0 & 0x3FFFFFC0) >> 6;
   tlb_e[idx].pfn_odd = (core_EntryLo1 & 0x3FFFFFC0) >> 6;
   tlb_e[idx].c_even = (core_EntryLo0 & 0x38) >> 3;
   tlb_e[idx].c_odd = (core_EntryLo1 & 0x38) >> 3;
   tlb_e[idx].d_even = (core_EntryLo0 & 0x4) >> 2;
   tlb_e[idx].d_odd = (core_EntryLo1 & 0x4) >> 2;
   tlb_e[idx].v_even = (core_EntryLo0 & 0x2) >> 1;
   tlb_e[idx].v_odd = (core_EntryLo1 & 0x2) >> 1;
   tlb_e[idx].asid = (core_EntryHi & 0xFF);
   tlb_e[idx].vpn2 = (core_EntryHi & 0xFFFFE000) >> 13;
   //tlb_e[idx].r = (core_EntryHi & 0xC000000000000000LL) >> 62;
   tlb_e[idx].mask = (core_PageMask & 0x1FFE000) >> 13;
   
   tlb_e[idx].start_even = tlb_e[idx].vpn2 << 13;
   tlb_e[idx].end_even = tlb_e[idx].start_even+
     (tlb_e[idx].mask << 12) + 0xFFF;
   tlb_e[idx].phys_even = tlb_e[idx].pfn_even << 12;
   
   tlb_e[idx].start_odd = tlb_e[idx].end_even+1;
   tlb_e[idx].end_odd = tlb_e[idx].start_odd+
     (tlb_e[idx].mask << 12) + 0xFFF;
   tlb_e[idx].phys_odd = tlb_e[idx].pfn_odd << 12;
   
   tlb_map(&tlb_e[idx]);
}

static void TLBWI(void)
{
   TLBWrite(core_Index&0x3F);
   interp_addr+=4;
}

static void TLBWR(void)
{
   update_count();
   core_Random = (core_Count/2 % (32 - core_Wired)) + core_Wired;
   TLBWrite(core_Random);
   interp_addr+=4;
}

static void TLBP(void)
{
   int i;
   core_Index |= 0x80000000;
   for (i=0; i<32; i++)
     {
    if (((tlb_e[i].vpn2 & (~tlb_e[i].mask)) ==
         (((core_EntryHi & 0xFFFFE000) >> 13) & (~tlb_e[i].mask))) &&
        ((tlb_e[i].g) ||
         (tlb_e[i].asid == (core_EntryHi & 0xFF))))
      {
         core_Index = i;
         break;
      }
     }
   interp_addr+=4;
}

static void ERET(void)
{
   update_count();
   if (core_Status & 0x4)
   {
     g_core->logger->error("error in ERET");
     stop=1;
   }
   else
   {
     core_Status &= 0xFFFFFFFD;
     interp_addr = core_EPC;
   }
   llbit = 0;
   check_interrupt();
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void (*interp_tlb[64])(void) =
{
   NI  , TLBR, TLBWI, NI, NI, NI, TLBWR, NI,
   TLBP, NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI,
   ERET, NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI
};

static void MFC0(void)
{
   switch(PC->f.r.nrd)
     {
      case 1:
        g_core->logger->error("MFC0 reading un-implemented core_Random register");
        stop=1;
      default:
        check_r0_rt();
        rrt32 = reg_cop0[PC->f.r.nrd];
        sign_extended(core_rrt);
     }
   interp_addr+=4;
}

static void MTC0(void)
{
   switch(PC->f.r.nrd)
     {
      case 0:    // core_Index
    core_Index = (unsigned int) core_rrt & 0x8000003F;
    if ((core_Index & 0x3F) > 31) 
      {
         g_core->logger->error("MTC0 writing core_Index register with TLB index > 31");
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
    interp_addr+=4;
    check_interrupt();
    update_count();
    interupt_unsafe_state = 1;
    if (next_interrupt <= core_Count) gen_interrupt();
    interupt_unsafe_state = 0;
    interp_addr-=4;
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
      case 16:  // core_Config_cop0
    core_Config = (unsigned int) core_rrt;
    break;
      case 18:  // core_WatchLo
    core_WatchLo = (unsigned int) core_rrt & 0xFFFFFFFF;
    break;
      case 19:  // core_WatchHi
    core_WatchHi = (unsigned int) core_rrt & 0xFFFFFFFF;
    break;
      case 27: // CacheErr
    break;
      case 28: // core_TagLo
    core_TagLo = (unsigned int) core_rrt & 0x0FFFFFC0;
    break;
      case 29: // core_TagHi
    core_TagHi =0;
    break;
      default:
    g_core->logger->error("Unknown MTC0 write: %d", PC->f.r.nrd);
    stop=1;
     }
   interp_addr+=4;
}

static void TLB(void)
{
   interp_tlb[(vr_op & 0x3F)]();
}

static void (*interp_cop0[32])(void) =
{
   MFC0, NI, NI, NI, MTC0, NI, NI, NI,
   NI  , NI, NI, NI, NI  , NI, NI, NI,
   TLB , NI, NI, NI, NI  , NI, NI, NI,
   NI  , NI, NI, NI, NI  , NI, NI, NI
};

static void BC1F(void)
{
   short local_immediate = core_iimmediate;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if ((FCR31 & 0x800000)==0)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   interp_addr+=4;
   delay_slot=1;
   prefetch();
   interp_ops[((vr_op >> 26) & 0x3F)]();
   update_count();
   delay_slot=0;
   if ((FCR31 & 0x800000)==0)
     interp_addr += (local_immediate-1)*4;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BC1T(void)
{
   short local_immediate = core_iimmediate;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if ((FCR31 & 0x800000)!=0)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   interp_addr+=4;
   delay_slot=1;
   prefetch();
   interp_ops[((vr_op >> 26) & 0x3F)]();
   update_count();
   delay_slot=0;
   if ((FCR31 & 0x800000)!=0)
     interp_addr += (local_immediate-1)*4;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BC1FL(void)
{
   short local_immediate = core_iimmediate;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if ((FCR31 & 0x800000)==0)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   if ((FCR31 & 0x800000)==0)
     {
    interp_addr+=4;
    delay_slot=1;
    prefetch();
    interp_ops[((vr_op >> 26) & 0x3F)]();
    update_count();
    delay_slot=0;
    interp_addr += (local_immediate-1)*4;
     }
   else
     interp_addr+=8;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BC1TL(void)
{
   short local_immediate = core_iimmediate;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if ((FCR31 & 0x800000)!=0)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   if ((FCR31 & 0x800000)!=0)
     {
    interp_addr+=4;
    delay_slot=1;
    prefetch();
    interp_ops[((vr_op >> 26) & 0x3F)]();
    update_count();
    delay_slot=0;
    interp_addr += (local_immediate-1)*4;
     }
   else
     interp_addr+=8;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void (*interp_cop1_bc[4])(void) =
{
   BC1F , BC1T,
   BC1FL, BC1TL
};

static void ADD_S(void)
{
   add_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft], reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void SUB_S(void)
{
   sub_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft], reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void MUL_S(void)
{
   mul_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft], reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void DIV_S(void)
{
   if((FCR31 & 0x400) && *reg_cop1_simple[core_cfft] == 0)
   {
     g_core->logger->error("DIV_S by 0");
   }
   div_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft], reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void SQRT_S(void)
{
   sqrt_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void ABS_S(void)
{
   abs_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void MOV_S(void)
{
   mov_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void NEG_S(void)
{
   neg_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void ROUND_L_S(void)
{
   round_l_s(reg_cop1_simple[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   interp_addr+=4;
}

static void TRUNC_L_S(void)
{
   trunc_l_s(reg_cop1_simple[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   interp_addr+=4;
}

static void CEIL_L_S(void)
{
   ceil_l_s(reg_cop1_simple[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   interp_addr+=4;
}

static void FLOOR_L_S(void)
{
   floor_l_s(reg_cop1_simple[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   interp_addr+=4;
}

static void ROUND_W_S(void)
{
   round_w_s(reg_cop1_simple[core_cffs], (int*)reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void TRUNC_W_S(void)
{
   trunc_w_s(reg_cop1_simple[core_cffs], (int*)reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void CEIL_W_S(void)
{
   ceil_w_s(reg_cop1_simple[core_cffs], (int*)reg_cop1_simple[core_cffd]); 
   interp_addr+=4;
}

static void FLOOR_W_S(void)
{
   floor_w_s(reg_cop1_simple[core_cffs], (int*)reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void CVT_D_S(void)
{
   cvt_d_s(reg_cop1_simple[core_cffs], reg_cop1_double[core_cffd]);
   interp_addr+=4;
}

static void CVT_W_S(void)
{
   cvt_w_s(reg_cop1_simple[core_cffs], (int*)reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void CVT_L_S(void)
{
   cvt_l_s(reg_cop1_simple[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   interp_addr+=4;
}

static void C_F_S(void)
{
   c_f_s();
   interp_addr+=4;
}

static void C_UN_S(void)
{
   c_un_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_EQ_S(void)
{
   c_eq_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_UEQ_S(void)
{
   c_ueq_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_OLT_S(void)
{
   c_olt_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_ULT_S(void)
{
   c_ult_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_OLE_S(void)
{
   c_ole_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_ULE_S(void)
{
   c_ule_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_SF_S(void)
{
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_sf_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_NGLE_S(void)
{
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_ngle_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_SEQ_S(void)
{
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_seq_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_NGL_S(void)
{
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_ngl_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_LT_S(void)
{
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_lt_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_NGE_S(void)
{
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_nge_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_LE_S(void)
{
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_le_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void C_NGT_S(void)
{
   if (isnan(*reg_cop1_simple[core_cffs]) || isnan(*reg_cop1_simple[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_ngt_s(reg_cop1_simple[core_cffs], reg_cop1_simple[core_cfft]);
   interp_addr+=4;
}

static void (*interp_cop1_s[64])(void) =
{
ADD_S    ,SUB_S    ,MUL_S   ,DIV_S    ,SQRT_S   ,ABS_S    ,MOV_S   ,NEG_S    ,
ROUND_L_S,TRUNC_L_S,CEIL_L_S,FLOOR_L_S,ROUND_W_S,TRUNC_W_S,CEIL_W_S,FLOOR_W_S,
NI       ,NI       ,NI      ,NI       ,NI       ,NI       ,NI      ,NI       ,
NI       ,NI       ,NI      ,NI       ,NI       ,NI       ,NI      ,NI       ,
NI       ,CVT_D_S  ,NI      ,NI       ,CVT_W_S  ,CVT_L_S  ,NI      ,NI       ,
NI       ,NI       ,NI      ,NI       ,NI       ,NI       ,NI      ,NI       ,
C_F_S    ,C_UN_S   ,C_EQ_S  ,C_UEQ_S  ,C_OLT_S  ,C_ULT_S  ,C_OLE_S ,C_ULE_S  ,
C_SF_S   ,C_NGLE_S ,C_SEQ_S ,C_NGL_S  ,C_LT_S   ,C_NGE_S  ,C_LE_S  ,C_NGT_S
};

static void ADD_D(void)
{
   add_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft], reg_cop1_double[core_cffd]);
   interp_addr+=4;
}

static void SUB_D(void)
{
   sub_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft], reg_cop1_double[core_cffd]);
   interp_addr+=4;
}

static void MUL_D(void)
{
   mul_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft], reg_cop1_double[core_cffd]);
   interp_addr+=4;
}

static void DIV_D(void)
{
   if((FCR31 & 0x400) && *reg_cop1_double[core_cfft] == 0)
     {
    //FCR31 |= 0x8020;
    /*FCR31 |= 0x8000;
    core_Cause = 15 << 2;
    exception_general();*/
    g_core->logger->error("DIV_D by 0");
    //return;
     }
   div_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft], reg_cop1_double[core_cffd]);
   interp_addr+=4;
}

static void SQRT_D(void)
{
   sqrt_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cffd]);
   interp_addr+=4;
}

static void ABS_D(void)
{
   abs_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cffd]);
   interp_addr+=4;
}

static void MOV_D(void)
{
   mov_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cffd]);
   interp_addr+=4;
}

static void NEG_D(void)
{
   neg_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cffd]);
   interp_addr+=4;
}

static void ROUND_L_D(void)
{
   round_l_d(reg_cop1_double[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   interp_addr+=4;
}

static void TRUNC_L_D(void)
{
   trunc_l_d(reg_cop1_double[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   interp_addr+=4;
}

static void CEIL_L_D(void)
{
   ceil_l_d(reg_cop1_double[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   interp_addr+=4;
}

static void FLOOR_L_D(void)
{
   floor_l_d(reg_cop1_double[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   interp_addr+=4;
}

static void ROUND_W_D(void)
{
   round_w_d(reg_cop1_double[core_cffs], (int*)reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void TRUNC_W_D(void)
{
   trunc_w_d(reg_cop1_double[core_cffs], (int*)reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void CEIL_W_D(void)
{
   ceil_w_d(reg_cop1_double[core_cffs], (int*)reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void FLOOR_W_D(void)
{
   floor_w_d(reg_cop1_double[core_cffs], ((int*)reg_cop1_simple[core_cffd]));
   interp_addr+=4;
}

static void CVT_S_D(void)
{
   cvt_s_d(reg_cop1_double[core_cffs], reg_cop1_simple[core_cffd]); 
   interp_addr+=4;
}

static void CVT_W_D(void)
{
   cvt_w_d(reg_cop1_double[core_cffs], (int*)reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void CVT_L_D(void)
{
   cvt_l_d(reg_cop1_double[core_cffs], (long long*)(reg_cop1_double[core_cffd]));
   interp_addr+=4;
}

static void C_F_D(void)
{
   c_f_d();
   interp_addr+=4;
}

static void C_UN_D(void)
{
   c_un_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_EQ_D(void)
{
   c_eq_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_UEQ_D(void)
{
   c_ueq_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_OLT_D(void)
{
   c_olt_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_ULT_D(void)
{
   c_ult_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_OLE_D(void)
{
   c_ole_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_ULE_D(void)
{
   c_ule_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_SF_D(void)
{
   if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
      stop=1;
   }
   c_sf_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_NGLE_D(void)
{
   if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_ngle_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_SEQ_D(void)
{
   if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_seq_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_NGL_D(void)
{
   if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_ngl_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_LT_D(void)
{
   if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_lt_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_NGE_D(void)
{
   if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_nge_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_LE_D(void)
{
   if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_le_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void C_NGT_D(void)
{
   if (isnan(*reg_cop1_double[core_cffs]) || isnan(*reg_cop1_double[core_cfft]))
   {
     g_core->logger->error("Invalid operation exception in C opcode");
     stop=1;
   }
   c_ngt_d(reg_cop1_double[core_cffs], reg_cop1_double[core_cfft]);
   interp_addr+=4;
}

static void (*interp_cop1_d[64])(void) =
{
ADD_D    ,SUB_D    ,MUL_D   ,DIV_D    ,SQRT_D   ,ABS_D    ,MOV_D   ,NEG_D    ,
ROUND_L_D,TRUNC_L_D,CEIL_L_D,FLOOR_L_D,ROUND_W_D,TRUNC_W_D,CEIL_W_D,FLOOR_W_D,
NI       ,NI       ,NI      ,NI       ,NI       ,NI       ,NI      ,NI       ,
NI       ,NI       ,NI      ,NI       ,NI       ,NI       ,NI      ,NI       ,
CVT_S_D  ,NI       ,NI      ,NI       ,CVT_W_D  ,CVT_L_D  ,NI      ,NI       ,
NI       ,NI       ,NI      ,NI       ,NI       ,NI       ,NI      ,NI       ,
C_F_D    ,C_UN_D   ,C_EQ_D  ,C_UEQ_D  ,C_OLT_D  ,C_ULT_D  ,C_OLE_D ,C_ULE_D  ,
C_SF_D   ,C_NGLE_D ,C_SEQ_D ,C_NGL_D  ,C_LT_D   ,C_NGE_D  ,C_LE_D  ,C_NGT_D
};

static void CVT_S_W(void)
{
   cvt_s_w(((int*)reg_cop1_simple[core_cffs]), reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void CVT_D_W(void)
{
   cvt_d_w(((int*)reg_cop1_simple[core_cffs]), reg_cop1_double[core_cffd]);
   interp_addr+=4;
}

static void (*interp_cop1_w[64])(void) =
{
   NI     , NI     , NI, NI, NI, NI, NI, NI,
   NI     , NI     , NI, NI, NI, NI, NI, NI,
   NI     , NI     , NI, NI, NI, NI, NI, NI,
   NI     , NI     , NI, NI, NI, NI, NI, NI,
   CVT_S_W, CVT_D_W, NI, NI, NI, NI, NI, NI,
   NI     , NI     , NI, NI, NI, NI, NI, NI,
   NI     , NI     , NI, NI, NI, NI, NI, NI,
   NI     , NI     , NI, NI, NI, NI, NI, NI
};

static void CVT_S_L(void)
{
   cvt_s_l((long long*)(reg_cop1_double[core_cffs]), reg_cop1_simple[core_cffd]);
   interp_addr+=4;
}

static void CVT_D_L(void)
{
   cvt_d_l((long long*)(reg_cop1_double[core_cffs]), reg_cop1_double[core_cffd]);
   interp_addr+=4;
}

static void (*interp_cop1_l[64])(void) =
{
   NI     , NI     , NI, NI, NI, NI, NI, NI,
   NI     , NI     , NI, NI, NI, NI, NI, NI,
   NI     , NI     , NI, NI, NI, NI, NI, NI,
   NI     , NI     , NI, NI, NI, NI, NI, NI,
   CVT_S_L, CVT_D_L, NI, NI, NI, NI, NI, NI,
   NI     , NI     , NI, NI, NI, NI, NI, NI,
   NI     , NI     , NI, NI, NI, NI, NI, NI,
   NI     , NI     , NI, NI, NI, NI, NI, NI
};

static void MFC1(void)
{
   check_r0_rt();
   rrt32 = *((int*)reg_cop1_simple[core_rfs]);
   sign_extended(core_rrt);
   interp_addr+=4;
}

static void DMFC1(void)
{
   check_r0_rt();
   core_rrt = *((long long*)(reg_cop1_double[core_rfs]));
   interp_addr+=4;
}

static void CFC1(void)
{
   check_r0_rt();
   if (core_rfs==31)
     {
    rrt32 = FCR31;
    sign_extended(core_rrt);
     }
   if (core_rfs==0)
     {
    rrt32 = FCR0;
    sign_extended(core_rrt);
     }
   interp_addr+=4;
}

static void MTC1(void)
{
   *((int*)reg_cop1_simple[core_rfs]) = rrt32;
   interp_addr+=4;
}

static void DMTC1(void)
{
   *((long long*)reg_cop1_double[core_rfs]) = core_rrt;
   interp_addr+=4;
}

static void CTC1(void)
{
   if (core_rfs==31)
     FCR31 = rrt32;
   switch((FCR31 & 3))
     {
      case 0:
    rounding_mode = 0x33F; // Round to nearest, or to even if equidistant
    break;
      case 1:
    rounding_mode = 0xF3F; // Truncate (toward 0)
    break;
      case 2:
    rounding_mode = 0xB3F; // Round up (toward +infinity) 
    break;
      case 3:
    rounding_mode = 0x73F; // Round down (toward -infinity) 
    break;
     }
   //if ((FCR31 >> 7) & 0x1F) printf("FPU Exception enabled : %x\n",
//                 (int)((FCR31 >> 7) & 0x1F));
   interp_addr+=4;
}

static void BC(void)
{
   interp_cop1_bc[(vr_op >> 16) & 3]();
}

static void S(void)
{
   interp_cop1_s[(vr_op & 0x3F)]();
}

static void D(void)
{
   interp_cop1_d[(vr_op & 0x3F)]();
}

static void W(void)
{
   interp_cop1_w[(vr_op & 0x3F)]();
}

static void L(void)
{
   interp_cop1_l[(vr_op & 0x3F)]();
}

static void (*interp_cop1[32])(void) =
{
   MFC1, DMFC1, CFC1, NI, MTC1, DMTC1, CTC1, NI,
   BC  , NI   , NI  , NI, NI  , NI   , NI  , NI,
   S   , D    , NI  , NI, W   , L    , NI  , NI,
   NI  , NI   , NI  , NI, NI  , NI   , NI  , NI
};

static void SPECIAL(void)
{
   interp_special[(vr_op & 0x3F)]();
}

static void REGIMM(void)
{
   interp_regimm[((vr_op >> 16) & 0x1F)]();
}

static void J(void)
{
   unsigned int naddr = (PC->f.j.inst_index<<2) | (interp_addr & 0xF0000000);
   if (naddr == interp_addr)
     {
    if (probe_nop(interp_addr+4))
      {
         update_count();
         skip = next_interrupt - core_Count;
         if (skip > 3) 
           {
          core_Count += (skip & 0xFFFFFFFC);
          return;
           }
      }
     }
   interp_addr+=4;
   delay_slot=1;
   prefetch();
   interp_ops[((vr_op >> 26) & 0x3F)]();
   update_count();
   delay_slot=0;
   interp_addr = naddr;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void JAL(void)
{
   unsigned int naddr = (PC->f.j.inst_index<<2) | (interp_addr & 0xF0000000);
   if (naddr == interp_addr)
     {
    if (probe_nop(interp_addr+4))
      {
         update_count();
         skip = next_interrupt - core_Count;
         if (skip > 3) 
           {
          core_Count += (skip & 0xFFFFFFFC);
          return;
           }
      }
     }
   interp_addr+=4;
   delay_slot=1;
   prefetch();
   interp_ops[((vr_op >> 26) & 0x3F)]();
   update_count();
   delay_slot=0;
   if (!skip_jump)
     {
    reg[31]=interp_addr;
    sign_extended(reg[31]);
    
    interp_addr = naddr;
     }
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BEQ(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   local_rt = core_irt;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if (local_rs == local_rt)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   interp_addr+=4;
   delay_slot=1;
   prefetch();
   interp_ops[((vr_op >> 26) & 0x3F)]();
   update_count();
   delay_slot=0;
   if (local_rs == local_rt)
     interp_addr += (local_immediate-1)*4;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BNE(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   local_rt = core_irt;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if (local_rs != local_rt)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   interp_addr+=4;
   delay_slot=1;
   prefetch();
   interp_ops[((vr_op >> 26) & 0x3F)]();
   update_count();
   delay_slot=0;
   if (local_rs != local_rt)
     interp_addr += (local_immediate-1)*4;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BLEZ(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if (local_rs <= 0)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   interp_addr+=4;
   delay_slot=1;
   prefetch();
   interp_ops[((vr_op >> 26) & 0x3F)]();
   update_count();
   delay_slot=0;
   if (local_rs <= 0)
     interp_addr += (local_immediate-1)*4;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BGTZ(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if (local_rs > 0)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   interp_addr+=4;
   delay_slot=1;
   prefetch();
   interp_ops[((vr_op >> 26) & 0x3F)]();
   update_count();
   delay_slot=0;
   if (local_rs > 0)
     interp_addr += (local_immediate-1)*4;
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void ADDI(void)
{
   check_r0_irt();
   irt32 = irs32 + core_iimmediate;
   sign_extended(core_irt);
   interp_addr+=4;
}

static void ADDIU(void)
{
   check_r0_irt();
   irt32 = irs32 + core_iimmediate;
   sign_extended(core_irt);
   interp_addr+=4;
}

static void SLTI(void)
{
   check_r0_irt();
   if (core_irs < core_iimmediate) core_irt = 1;
   else core_irt = 0;
   interp_addr+=4;
}

static void SLTIU(void)
{
   check_r0_irt();
   if ((unsigned long long)core_irs < (unsigned long long)((long long)core_iimmediate))
     core_irt = 1;
   else core_irt = 0;
   interp_addr+=4;
}

static void ANDI(void)
{
   check_r0_irt();
   core_irt = core_irs & (unsigned short)core_iimmediate;
   interp_addr+=4;
}

static void ORI(void)
{
   check_r0_irt();
   core_irt = core_irs | (unsigned short)core_iimmediate;
   interp_addr+=4;
}

static void XORI(void)
{
   check_r0_irt();
   core_irt = core_irs ^ (unsigned short)core_iimmediate;
   interp_addr+=4;
}

static void LUI(void)
{
   check_r0_irt();
   irt32 = core_iimmediate << 16;
   sign_extended(core_irt);
   interp_addr+=4;
}

static void COP0(void)
{
   interp_cop0[((vr_op >> 21) & 0x1F)]();
}

static void COP1(void)
{
   if (check_cop1_unusable()) return;
   interp_cop1[((vr_op >> 21) & 0x1F)]();
}

static void BEQL(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   local_rt = core_irt;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if (core_irs == core_irt)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   if (local_rs == local_rt)
     {
    interp_addr+=4;
    delay_slot=1;
    prefetch();
    interp_ops[((vr_op >> 26) & 0x3F)]();
    update_count();
    delay_slot=0;
    interp_addr += (local_immediate-1)*4;
     }
   else
     {
    interp_addr+=8;
    update_count();
     }
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BNEL(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   local_rt = core_irt;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if (core_irs != core_irt)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   if (local_rs != local_rt)
     {
    interp_addr+=4;
    delay_slot=1;
    prefetch();
    interp_ops[((vr_op >> 26) & 0x3F)]();
    update_count();
    delay_slot=0;
    interp_addr += (local_immediate-1)*4;
     }
   else
     {
    interp_addr+=8;
    update_count();
     }
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BLEZL(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if (core_irs <= 0)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   if (local_rs <= 0)
     {
    interp_addr+=4;
    delay_slot=1;
    prefetch();
    interp_ops[((vr_op >> 26) & 0x3F)]();
    update_count();
    delay_slot=0;
    interp_addr += (local_immediate-1)*4;
     }
   else
     {
    interp_addr+=8;
    update_count();
     }
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void BGTZL(void)
{
   short local_immediate = core_iimmediate;
   local_rs = core_irs;
   if ((interp_addr + (local_immediate+1)*4) == interp_addr)
     if (core_irs > 0)
       {
      if (probe_nop(interp_addr+4))
        {
           update_count();
           skip = next_interrupt - core_Count;
           if (skip > 3) 
         {
            core_Count += (skip & 0xFFFFFFFC);
            return;
         }
        }
       }
   if (local_rs > 0)
     {
    interp_addr+=4;
    delay_slot=1;
    prefetch();
    interp_ops[((vr_op >> 26) & 0x3F)]();
    update_count();
    delay_slot=0;
    interp_addr += (local_immediate-1)*4;
     }
   else
     {
    interp_addr+=8;
    update_count();
     }
   last_addr = interp_addr;
   if (next_interrupt <= core_Count) gen_interrupt();
}

static void DADDI(void)
{
   check_r0_irt();
   core_irt = core_irs + core_iimmediate;
   interp_addr+=4;
}

static void DADDIU(void)
{
   check_r0_irt();
   core_irt = core_irs + core_iimmediate;
   interp_addr+=4;
}

static void LDL(void)
{
   unsigned long long int word = 0;
   check_r0_irt();
   interp_addr+=4;
   switch ((core_iimmediate + irs32) & 7)
     {
      case 0:
    address = core_iimmediate + irs32;
    rdword = (unsigned long long *) &core_irt;
    read_dword_in_memory();
    break;
      case 1:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFF) | (word << 8);
    break;
      case 2:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFFFF) | (word << 16);
    break;
      case 3:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFFFFFF) | (word << 24);
    break;
      case 4:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFFFFFFFF) | (word << 32);
    break;
      case 5:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFFFFFFFFFFLL) | (word << 40);
    break;
      case 6:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFFFFFFFFFFFFLL) | (word << 48);
    break;
      case 7:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFFFFFFFFFFFFFFLL) | (word << 56);
    break;
     }
}

static void LDR(void)
{
   unsigned long long int word = 0;
   check_r0_irt();
   interp_addr+=4;
   switch ((core_iimmediate + irs32) & 7)
     {
      case 0:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFFFFFFFFFFFFFF00LL) | (word >> 56);
    break;
      case 1:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFFFFFFFFFFFF0000LL) | (word >> 48);
    break;
      case 2:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFFFFFFFFFF000000LL) | (word >> 40);
    break;
      case 3:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFFFFFFFF00000000LL) | (word >> 32);
    break;
      case 4:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFFFFFF0000000000LL) | (word >> 24);
    break;
      case 5:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFFFF000000000000LL) | (word >> 16);
    break;
      case 6:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &word;
    read_dword_in_memory();
    core_irt = (core_irt & 0xFF00000000000000LL) | (word >> 8);
    break;
      case 7:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = (unsigned long long *) &core_irt;
    read_dword_in_memory();
    break;
     }
}

static void LB(void)
{
   check_r0_irt();
   interp_addr+=4;
   address = core_iimmediate + irs32;
   rdword = (unsigned long long *) &core_irt;
   read_byte_in_memory();
   sign_extendedb(core_irt);
}

static void LH(void)
{
   check_r0_irt();
   interp_addr+=4;
   address = core_iimmediate + irs32;
   rdword = (unsigned long long *) &core_irt;
   read_hword_in_memory();
   sign_extendedh(core_irt);
}

static void LWL(void)
{
   unsigned long long int word = 0;
   check_r0_irt();
   interp_addr+=4;
   switch ((core_iimmediate + irs32) & 3)
     {
      case 0:
    address = core_iimmediate + irs32;
    rdword = (unsigned long long *) &core_irt;
    read_word_in_memory();
    break;
      case 1:
    address = (core_iimmediate + irs32) & 0xFFFFFFFC;
    rdword = &word;
    read_word_in_memory();
    core_irt = (core_irt & 0xFF) | (word << 8);
    break;
      case 2:
    address = (core_iimmediate + irs32) & 0xFFFFFFFC;
    rdword = &word;
    read_word_in_memory();
    core_irt = (core_irt & 0xFFFF) | (word << 16);
    break;
      case 3:
    address = (core_iimmediate + irs32) & 0xFFFFFFFC;
    rdword = &word;
    read_word_in_memory();
    core_irt = (core_irt & 0xFFFFFF) | (word << 24);
    break;
     }
   sign_extended(core_irt);
}

static void LW(void)
{
   check_r0_irt();
   address = core_iimmediate + irs32;
   rdword = (unsigned long long *) &core_irt;
   interp_addr+=4;
   read_word_in_memory();
   sign_extended(core_irt);
}

static void LBU(void)
{
   check_r0_irt();
   interp_addr+=4;
   address = core_iimmediate + irs32;
   rdword = (unsigned long long *) &core_irt;
   read_byte_in_memory();
}

static void LHU(void)
{
   check_r0_irt();
   interp_addr+=4;
   address = core_iimmediate + irs32;
   rdword = (unsigned long long *) &core_irt;
   read_hword_in_memory();
}

static void LWR(void)
{
   unsigned long long int word = 0;
   check_r0_irt();
   interp_addr+=4;
   switch ((core_iimmediate + irs32) & 3)
     {
      case 0:
    address = (core_iimmediate + irs32) & 0xFFFFFFFC;
    rdword = &word;
    read_word_in_memory();
    core_irt = (core_irt & 0xFFFFFFFFFFFFFF00LL) | ((word >> 24) & 0xFF);
    break;
      case 1:
    address = (core_iimmediate + irs32) & 0xFFFFFFFC;
    rdword = &word;
    read_word_in_memory();
    core_irt = (core_irt & 0xFFFFFFFFFFFF0000LL) | ((word >> 16) & 0xFFFF);
    break;
      case 2:
    address = (core_iimmediate + irs32) & 0xFFFFFFFC;
    rdword = &word;
    read_word_in_memory();
    core_irt = (core_irt & 0xFFFFFFFFFF000000LL) | ((word >> 8) & 0xFFFFFF);
    break;
      case 3:
    address = (core_iimmediate + irs32) & 0xFFFFFFFC;
    rdword = (unsigned long long *) &core_irt;
    read_word_in_memory();
    sign_extended(core_irt);
     }
}

static void LWU(void)
{
   check_r0_irt();
   address = core_iimmediate + irs32;
   rdword = (unsigned long long *) &core_irt;
   interp_addr+=4;
   read_word_in_memory();
}

static void SB(void)
{
   interp_addr+=4;
   address = core_iimmediate + irs32;
   g_byte = (unsigned char)(core_irt & 0xFF);
   write_byte_in_memory();
}

static void SH(void)
{
   interp_addr+=4;
   address = core_iimmediate + irs32;
   hword = (unsigned short)(core_irt & 0xFFFF);
   write_hword_in_memory();
}

static void SWL(void)
{
   unsigned long long int old_word = 0;
   interp_addr+=4;
   switch ((core_iimmediate + irs32) & 3)
     {
      case 0:
    address = (core_iimmediate + irs32) & 0xFFFFFFFC;
    word = (unsigned int)core_irt;
    write_word_in_memory();
    break;
      case 1:
    address = (core_iimmediate + irs32) & 0xFFFFFFFC;
    rdword = &old_word;
    read_word_in_memory();
    word = ((unsigned int)core_irt >> 8) | ((unsigned int) old_word & 0xFF000000);
    write_word_in_memory();
    break;
      case 2:
    address = (core_iimmediate + irs32) & 0xFFFFFFFC;
    rdword = &old_word;
    read_word_in_memory();
    word = ((unsigned int)core_irt >> 16) | ((unsigned int) old_word & 0xFFFF0000);
    write_word_in_memory();
    break;
      case 3:
    address = core_iimmediate + irs32;
    g_byte = (unsigned char)(core_irt >> 24);
    write_byte_in_memory();
    break;
     }
}

static void SW(void)
{
   interp_addr+=4;
   address = core_iimmediate + irs32;
   word = (unsigned int)(core_irt & 0xFFFFFFFF);
   write_word_in_memory();
}

static void SDL(void)
{
   unsigned long long int old_word = 0;
   interp_addr+=4;
   switch ((core_iimmediate + irs32) & 7)
     {
      case 0:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    dword = core_irt;
    write_dword_in_memory();
    break;
      case 1:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &old_word;
    read_dword_in_memory();
    dword = ((unsigned long long)core_irt >> 8)|(old_word & 0xFF00000000000000LL);
    write_dword_in_memory();
    break;
      case 2:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &old_word;
    read_dword_in_memory();
    dword = ((unsigned long long)core_irt >> 16)|(old_word & 0xFFFF000000000000LL);
    write_dword_in_memory();
    break;
      case 3:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &old_word;
    read_dword_in_memory();
    dword = ((unsigned long long)core_irt >> 24)|(old_word & 0xFFFFFF0000000000LL);
    write_dword_in_memory();
    break;
      case 4:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &old_word;
    read_dword_in_memory();
    dword = ((unsigned long long)core_irt >> 32)|(old_word & 0xFFFFFFFF00000000LL);
    write_dword_in_memory();
    break;
      case 5:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &old_word;
    read_dword_in_memory();
    dword = ((unsigned long long)core_irt >> 40)|(old_word & 0xFFFFFFFFFF000000LL);
    write_dword_in_memory();
    break;
      case 6:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &old_word;
    read_dword_in_memory();
    dword = ((unsigned long long)core_irt >> 48)|(old_word & 0xFFFFFFFFFFFF0000LL);
    write_dword_in_memory();
    break;
      case 7:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &old_word;
    read_dword_in_memory();
    dword = ((unsigned long long)core_irt >> 56)|(old_word & 0xFFFFFFFFFFFFFF00LL);
    write_dword_in_memory();
    break;
     }
}

static void SDR(void)
{
   unsigned long long int old_word = 0;
   interp_addr+=4;
   switch ((core_iimmediate + irs32) & 7)
     {
      case 0:
    address = core_iimmediate + irs32;
    rdword = &old_word;
    read_dword_in_memory();
    dword = (core_irt << 56) | (old_word & 0x00FFFFFFFFFFFFFFLL);
    write_dword_in_memory();
    break;
      case 1:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &old_word;
    read_dword_in_memory();
    dword = (core_irt << 48) | (old_word & 0x0000FFFFFFFFFFFFLL);
    write_dword_in_memory();
    break;
      case 2:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &old_word;
    read_dword_in_memory();
    dword = (core_irt << 40) | (old_word & 0x000000FFFFFFFFFFLL);
    write_dword_in_memory();
    break;
      case 3:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &old_word;
    read_dword_in_memory();
    dword = (core_irt << 32) | (old_word & 0x00000000FFFFFFFFLL);
    write_dword_in_memory();
    break;
      case 4:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &old_word;
    read_dword_in_memory();
    dword = (core_irt << 24) | (old_word & 0x0000000000FFFFFFLL);
    write_dword_in_memory();
    break;
      case 5:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &old_word;
    read_dword_in_memory();
    dword = (core_irt << 16) | (old_word & 0x000000000000FFFFLL);
    write_dword_in_memory();
    break;
      case 6:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    rdword = &old_word;
    read_dword_in_memory();
    dword = (core_irt << 8) | (old_word & 0x00000000000000FFLL);
    write_dword_in_memory();
    break;
      case 7:
    address = (core_iimmediate + irs32) & 0xFFFFFFF8;
    dword = core_irt;
    write_dword_in_memory();
    break;
     }
}

static void SWR(void)
{
   unsigned long long int old_word = 0;
   interp_addr+=4;
   switch ((core_iimmediate + irs32) & 3)
     {
      case 0:
    address = core_iimmediate + irs32;
    rdword = &old_word;
    read_word_in_memory();
    word = ((unsigned int)core_irt << 24) | ((unsigned int) old_word & 0x00FFFFFF);
    write_word_in_memory();
    break;
      case 1:
    address = (core_iimmediate + irs32) & 0xFFFFFFFC;
    rdword = &old_word;
    read_word_in_memory();
    word = ((unsigned int)core_irt << 16) | ((unsigned int) old_word & 0x0000FFFF);
    write_word_in_memory();
    break;
      case 2:
    address = (core_iimmediate + irs32) & 0xFFFFFFFC;
    rdword = &old_word;
    read_word_in_memory();
    word = ((unsigned int)core_irt << 8) | ((unsigned int) old_word & 0x000000FF);
    write_word_in_memory();
    break;
      case 3:
    address = (core_iimmediate + irs32) & 0xFFFFFFFC;
    word = (unsigned int)core_irt;
    write_word_in_memory();
    break;
     }
}

static void CACHE(void)
{
   interp_addr+=4;
}

static void LL(void)
{
   check_r0_irt();
   address = core_iimmediate + irs32;
   rdword = (unsigned long long *) &core_irt;
   interp_addr+=4;
   read_word_in_memory();
   sign_extended(core_irt);
   llbit = 1;
}

static void LWC1(void)
{
   unsigned long long int temp;
   if (check_cop1_unusable()) return;
   interp_addr+=4;
   address = (unsigned int) (core_lfoffset + reg[core_lfbase]);
   rdword = &temp;
   read_word_in_memory();
   *((int*)reg_cop1_simple[core_lfft]) = (int) *rdword;
}

static void LDC1(void)
{
   if (check_cop1_unusable()) return;
   interp_addr+=4;
   address = (unsigned int) (core_lfoffset + reg[core_lfbase]);
   rdword = (unsigned long long*) reg_cop1_double[core_lfft];
   read_dword_in_memory();
}

static void LD(void)
{
   check_r0_irt();
   interp_addr+=4;
   address = core_iimmediate + irs32;
   rdword = (unsigned long long *) &core_irt;
   read_dword_in_memory();
}

static void SC(void)
{
   check_r0_irt();
   interp_addr+=4;
   if(llbit)
     {
    address = core_iimmediate + irs32;
    word = (unsigned int)(core_irt & 0xFFFFFFFF);
    write_word_in_memory();
    llbit = 0;
    core_irt = 1;
     }
   else
     {
    core_irt = 0;
     }
}

static void SWC1(void)
{
   if (check_cop1_unusable()) return;
   interp_addr+=4;
   address = (unsigned int) (core_lfoffset + reg[core_lfbase]);
   word = *((int*)reg_cop1_simple[core_lfft]);
   write_word_in_memory();
}

static void SDC1(void)
{
   if (check_cop1_unusable()) return;
   interp_addr+=4;
   address = (unsigned int) (core_lfoffset + reg[core_lfbase]);
   dword = *((unsigned long long*)reg_cop1_double[core_lfft]);
   write_dword_in_memory();
}

static void SD(void)
{
   interp_addr+=4;
   address = core_iimmediate + irs32;
   dword = core_irt;
   write_dword_in_memory();
}

static void (*interp_ops[64])(void) =
{
   SPECIAL, REGIMM, J   , JAL  , BEQ , BNE , BLEZ , BGTZ ,
   ADDI   , ADDIU , SLTI, SLTIU, ANDI, ORI , XORI , LUI  ,
   COP0   , COP1  , NI  , NI   , BEQL, BNEL, BLEZL, BGTZL,
   DADDI  , DADDIU, LDL , LDR  , NI  , NI  , NI   , NI   ,
   LB     , LH    , LWL , LW   , LBU , LHU , LWR  , LWU  ,
   SB     , SH    , SWL , SW   , SDL , SDR , SWR  , CACHE,
   LL     , LWC1  , NI  , NI   , NI  , LDC1, NI   , LD   ,
   SC     , SWC1  , NI  , NI   , NI  , SDC1, NI   , SD
};

static void prefetch(void)
{
  unsigned int *mem = fast_mem_access(interp_addr);
  if (mem != NULL)
  {
     vr_op = *mem;
     prefetch_opcode(vr_op);
  }
  else
  {
     g_core->logger->error("prefetch() execute address :%x", (int)interp_addr);
     stop=1;
  }
}

void pure_interpreter(void)
{
   interp_addr = 0xa4000040;
   stop=0;
   PC = (precomp_instr *) malloc(sizeof(precomp_instr));
   PC->addr = last_addr = interp_addr;

/*#ifdef DBG
         if (g_DebuggerActive)
           update_debugger(PC->addr);
#endif*/

   while (!stop)
   {
     prefetch();
#ifdef COMPARE_CORE
     CoreCompareCallback();
#endif
#ifdef DBG
     PC->addr = interp_addr;
     if (g_DebuggerActive) update_debugger(PC->addr);
#endif
     interp_ops[((vr_op >> 26) & 0x3F)]();
   }
   PC->addr = interp_addr;
}

