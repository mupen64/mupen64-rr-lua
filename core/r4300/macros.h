/**
 * Mupen64 - macros.h
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

#ifndef MACROS_H
#define MACROS_H

#define sign_extended(a) a = (int64_t)((int32_t)a)
#define sign_extendedb(a) a = (int64_t)((char)a)
#define sign_extendedh(a) a = (int64_t)((int16_t)a)

#define core_rrt *PC->f.r.rt
#define core_rrd *PC->f.r.rd
#define core_rfs PC->f.r.nrd
#define core_rrs *PC->f.r.rs
#define core_rsa PC->f.r.sa
#define core_irt *PC->f.i.rt
#define core_ioffset PC->f.i.immediate
#define core_iimmediate PC->f.i.immediate
#define core_lsaddr (PC - 1)->f.i.immediate + (*(PC - 1)->f.i.rs)
#define core_lsrt *(PC - 1)->f.i.rt
#define core_irs *PC->f.i.rs
#define core_ibase *PC->f.i.rs
#define core_jinst_index PC->f.j.inst_index
#define core_lfbase PC->f.lf.base
#define core_lfft PC->f.lf.ft
#define core_lfoffset PC->f.lf.offset
#define core_lslfaddr (PC - 1)->f.lf.offset + reg[(PC - 1)->f.lf.base]
#define core_lslfft (PC - 1)->f.lf.ft
#define core_cfft PC->f.cf.ft
#define core_cffs PC->f.cf.fs
#define core_cffd PC->f.cf.fd

// 32 bits macros
#ifndef _BIG_ENDIAN
#define rrt32 *((int32_t *)PC->f.r.rt)
#define rrd32 *((int32_t *)PC->f.r.rd)
#define rrs32 *((int32_t *)PC->f.r.rs)
#define irs32 *((int32_t *)PC->f.i.rs)
#define irt32 *((int32_t *)PC->f.i.rt)
#else
#define rrt32 *((int32_t *)PC->f.r.rt + 1)
#define rrd32 *((int32_t *)PC->f.r.rd + 1)
#define rrs32 *((int32_t *)PC->f.r.rs + 1)
#define irs32 *((int32_t *)PC->f.i.rs + 1)
#define irt32 *((int32_t *)PC->f.i.rt + 1)
#endif

#define check_PC                                                               \
  if (PC->addr == actual->fin) {                                               \
    g_core_logger->error("changement de block");                               \
    stop = 1;                                                                  \
  }

// cop0 macros
#define core_Index reg_cop0[0]
#define core_Random reg_cop0[1]
#define core_EntryLo0 reg_cop0[2]
#define core_EntryLo1 reg_cop0[3]
#define core_Context reg_cop0[4]
#define core_PageMask reg_cop0[5]
#define core_Wired reg_cop0[6]
#define core_BadVAddr reg_cop0[8]
#define core_Count reg_cop0[9]
#define core_EntryHi reg_cop0[10]
#define core_Compare reg_cop0[11]
#define core_Status reg_cop0[12]
#define core_Cause reg_cop0[13]
#define core_EPC reg_cop0[14]
#define core_PRevID reg_cop0[15]
#define core_Config_cop0 reg_cop0[16]
#define core_LLAddr reg_cop0[17]
#define core_WatchLo reg_cop0[18]
#define core_WatchHi reg_cop0[19]
#define core_XContext reg_cop0[20]
#define core_PErr reg_cop0[26]
#define core_CacheErr reg_cop0[27]
#define core_TagLo reg_cop0[28]
#define core_TagHi reg_cop0[29]
#define core_ErrorEPC reg_cop0[30]

#define set_rounding() __asm { fldcw rounding_mode }
#define set_trunc() __asm { fldcw trunc_mode }
#define set_round_to_nearest() __asm { fldcw round_mode }
#define set_ceil() __asm { fldcw ceil_mode }
#define set_floor() __asm { fldcw floor_mode }
#define clear_x87_exceptions() __asm { fclex }
#define read_x87_status_word() __asm { fstsw x87_status_word }

#ifdef _M_X64

#define FLOAT_CONVERT(_, __)

#else

// asm converter that respects rounding modes
#define FLOAT_CONVERT(input_width, output_width)                               \
  __asm { \
__asm mov eax, src \
__asm fld input_width ptr [eax] \
__asm mov eax, dest \
__asm fistp output_width ptr [eax] }

#endif

#define FLOAT_CONVERT_L_S(s, d)                                                \
  {                                                                            \
    float *src = s;                                                            \
    int64_t *dest = (int64_t *)d;                                              \
    FLOAT_CONVERT(dword, qword);                                               \
  }
#define FLOAT_CONVERT_W_S(s, d)                                                \
  {                                                                            \
    float *src = s;                                                            \
    int32_t *dest = (int32_t *)d;                                              \
    FLOAT_CONVERT(dword, dword);                                               \
  }
#define FLOAT_CONVERT_L_D(s, d)                                                \
  {                                                                            \
    double *src = s;                                                           \
    int64_t *dest = (int64_t *)d;                                              \
    FLOAT_CONVERT(qword, dword);                                               \
  }
#define FLOAT_CONVERT_W_D(s, d)                                                \
  {                                                                            \
    double *src = s;                                                           \
    int32_t *dest = (int32_t *)d;                                              \
    FLOAT_CONVERT(qword, qword);                                               \
  }
#endif
