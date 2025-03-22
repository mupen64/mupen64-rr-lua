/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#ifdef _DEBUG
#define VR_PROFILE (1)
#endif

#include <r4300/recomp.h>
#include <memory/tlb.h>
#include <r4300/rom.h>
#include <include/core_types.h>

extern std::atomic<size_t> frame_advance_outstanding;
extern std::recursive_mutex g_emu_cs;

extern precomp_instr* PC;
extern uint32_t vr_op;

extern precomp_block *blocks[0x100000], *actual;
extern void (*interp_ops[64])(void);
extern int32_t fast_memory;
extern bool g_vr_beq_ignore_jmp;
extern volatile bool emu_launched;
extern volatile bool emu_paused;
extern volatile bool core_executing;
extern size_t g_total_frames;
extern int32_t stop, llbit;
extern int64_t reg[32], hi, lo;
extern int64_t local_rs, local_rt;
extern uint32_t reg_cop0[32];
extern int32_t local_rs32, local_rt32;
extern uint32_t jump_target;
extern double* reg_cop1_double[32];
extern float* reg_cop1_simple[32];
extern int32_t reg_cop1_fgr_32[32];
extern int64_t reg_cop1_fgr_64[32];
extern int32_t FCR0, FCR31;
extern tlb tlb_e[32];
extern uint32_t delay_slot, skip_jump, dyna_interp;
extern uint64_t debug_count;
extern uint32_t dynacore;
extern uint32_t interpcore;
extern uint32_t next_interrupt, CIC_Chip;
extern int32_t rounding_mode, trunc_mode, round_mode, ceil_mode, floor_mode;
extern int16_t x87_status_word;
extern uint32_t last_addr, interp_addr;
extern char invalid_code[0x100000];
extern uint32_t jump_to_address;
extern std::atomic<bool> screen_invalidated;
extern int32_t vi_field;
extern uint32_t next_vi;
extern bool g_vr_fast_forward;
extern bool g_vr_frame_skipped;
extern core_system_type g_sys_type;

extern FILE* g_eeprom_file;
extern FILE* g_sram_file;
extern FILE* g_fram_file;
extern FILE* g_mpak_file;

extern bool g_vr_benchmark_enabled;

void pure_interpreter();
extern void jump_to_func();
void update_count();
int32_t check_cop1_unusable();
void terminate_emu();

core_result vr_reset_rom_impl(bool reset_save_data, bool stop_vcr, bool skip_reset_recording_check = false);

void* malloc_exec(size_t size);
void* realloc_exec(void* ptr, size_t oldsize, size_t newsize);
void free_exec(void* ptr);

#define jump_to(a)           \
    {                        \
        jump_to_address = a; \
        jump_to_func();      \
    }
