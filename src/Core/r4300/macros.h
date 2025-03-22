/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#ifdef _M_X64
#include <immintrin.h>
#include <stdint.h>
#include <fenv.h>
#include <intrin.h>
#include <stdio.h>
#endif

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
#define rrt32 *((int32_t*)PC->f.r.rt)
#define rrd32 *((int32_t*)PC->f.r.rd)
#define rrs32 *((int32_t*)PC->f.r.rs)
#define irs32 *((int32_t*)PC->f.i.rs)
#define irt32 *((int32_t*)PC->f.i.rt)
#else
#define rrt32 *((int32_t*)PC->f.r.rt + 1)
#define rrd32 *((int32_t*)PC->f.r.rd + 1)
#define rrs32 *((int32_t*)PC->f.r.rs + 1)
#define irs32 *((int32_t*)PC->f.i.rs + 1)
#define irt32 *((int32_t*)PC->f.i.rt + 1)
#endif

#define check_PC                                   \
    if (PC->addr == actual->fin)                   \
    {                                              \
        g_core->log_error(L"changement de block"); \
        stop = 1;                                  \
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

#define CODE_BLOCK_SIZE 5000
#define JUMP_TABLE_SIZE 1000

#ifdef _M_X64

#define MUP_ROUND_TRUNC FE_TOWARDZERO
#define MUP_ROUND_NEAREST FE_TONEAREST
#define MUP_ROUND_CEIL FE_UPWARD
#define MUP_ROUND_FLOOR FE_DOWNWARD

#define set_rounding() fesetround(rounding_mode)
#define set_trunc() fesetround(FE_TOWARDZERO)
#define set_round_to_nearest() fesetround(FE_TONEAREST)
#define set_ceil() fesetround(FE_UPWARD)
#define set_floor() fesetround(FE_DOWNWARD)

#define clear_x87_exceptions() feclearexcept(FE_ALL_EXCEPT)

#define read_x87_status_word() fegetexceptflag()

static int64_t convert_float_to_int64(float f)
{
    return _mm_cvtss_si64(_mm_set_ss(f));
}

static int32_t convert_float_to_int32(float f)
{
    return _mm_cvtss_si32(_mm_set_ss(f));
}

static int64_t convert_double_to_int64(double d)
{
    return _mm_cvtsd_si64(_mm_set_sd(d));
}

static int32_t convert_double_to_int32(double d)
{
    return _mm_cvtsd_si32(_mm_set_sd(d));
}

#define FLOAT_CONVERT_L_S(s, d) (*(int64_t*)(d) = convert_float_to_int64(*(float*)(s)))
#define FLOAT_CONVERT_W_S(s, d) (*(int32_t*)(d) = convert_float_to_int32(*(float*)(s)))
#define FLOAT_CONVERT_L_D(s, d) (*(int64_t*)(d) = convert_double_to_int64(*(double*)(s)))
#define FLOAT_CONVERT_W_D(s, d) (*(int32_t*)(d) = convert_double_to_int32(*(double*)(s)))

#else

#define MUP_ROUND_TRUNC 0xE3F
#define MUP_ROUND_NEAREST 0x23F
#define MUP_ROUND_CEIL 0xA3F
#define MUP_ROUND_FLOOR 0x63F

#define set_rounding() __asm { fldcw rounding_mode }
#define set_trunc() __asm { fldcw trunc_mode }
#define set_round_to_nearest() __asm { fldcw round_mode }
#define set_ceil() __asm { fldcw ceil_mode }
#define set_floor() __asm { fldcw floor_mode }
#define clear_x87_exceptions() __asm { fclex }
#define read_x87_status_word() __asm { fstsw x87_status_word }

// asm converter that respects rounding modes
#define FLOAT_CONVERT(input_width, output_width) __asm { \
__asm mov eax, src \
__asm fld input_width ptr [eax] \
__asm mov eax, dest \
__asm fistp output_width ptr [eax] }

#define FLOAT_CONVERT_L_S(s, d)      \
    {                                \
        float* src = s;              \
        int64_t* dest = (int64_t*)d; \
        FLOAT_CONVERT(dword, qword); \
    }
#define FLOAT_CONVERT_W_S(s, d)      \
    {                                \
        float* src = s;              \
        int32_t* dest = (int32_t*)d; \
        FLOAT_CONVERT(dword, dword); \
    }
#define FLOAT_CONVERT_L_D(s, d)      \
    {                                \
        double* src = s;             \
        int64_t* dest = (int64_t*)d; \
        FLOAT_CONVERT(qword, dword); \
    }
#define FLOAT_CONVERT_W_D(s, d)      \
    {                                \
        double* src = s;             \
        int32_t* dest = (int32_t*)d; \
        FLOAT_CONVERT(qword, qword); \
    }

#endif
