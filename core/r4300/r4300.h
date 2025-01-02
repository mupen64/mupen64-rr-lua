/**
 * Mupen64 - r4300.h
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

#ifdef _DEBUG
#define VR_PROFILE (1)
#endif

#include <core/memory/tlb.h>
#include <core/r4300/Plugin.hpp>
#include <core/r4300/recomp.h>
#include <core/r4300/rom.h>
#include <stdio.h>

extern precomp_instr* PC;
extern uint32_t vr_op;

extern precomp_block *blocks[0x100000], *actual;
extern void (*interp_ops[64])(void);
extern int32_t fast_memory;
extern std::unique_ptr<Plugin> video_plugin;
extern std::unique_ptr<Plugin> audio_plugin;
extern std::unique_ptr<Plugin> input_plugin;
extern std::unique_ptr<Plugin> rsp_plugin;
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
extern int32_t compare_core_mode;
extern bool g_vr_fast_forward;
extern bool g_vr_frame_skipped;

extern FILE* g_eeprom_file;
extern FILE* g_sram_file;
extern FILE* g_fram_file;
extern FILE* g_mpak_file;

extern bool g_vr_benchmark_enabled;
extern bool g_vr_no_frameskip;
extern std::atomic<int32_t> g_vr_wait_before_input_poll;

std::filesystem::path get_rom_path();

namespace Core
{
    /**
     * Starts a benchmark section. Must be stopped with <see
     * cref="stop_benchmark"/>.
     */
    void start_benchmark();

    /**
     * Stops the currently running benchmark section.
     * \return The amount of frames per real-time second during the benchmark. If no
     * benchmark is running, 0 is returned.
     */
    double stop_benchmark();
} // namespace Core

/**
 * \brief Resumes the emulator
 */
void resume_emu();

/**
 * \brief Pauses the emulator
 */
void pause_emu();

/**
 * \brief Starts the emulator
 * \param path Path to a rom or corresponding movie file
 * \param wait Whether the calling thread will wait for other core operations to
 * complete. When true, the Busy result is never returned. \return The operation
 * result
 */
CoreResult vr_start_rom(std::filesystem::path path, bool wait = false);

/**
 * \brief Stops the emulator
 * \param stop_vcr Whether all VCR operations will be stopped. When resetting
 * the ROM due to an in-movie restart, this needs to be false. \param wait
 * Whether the calling thread will wait for other core operations to complete.
 * When true, the Busy result is never returned. \return The operation result
 */
CoreResult vr_close_rom(bool stop_vcr = true, bool wait = false);

/**
 * \brief Resets the emulator
 * \param reset_save_data Whether save data (e.g.: EEPROM, SRAM, Mempak) will be
 * reset \param stop_vcr Whether all VCR operations will be stopped. When
 * resetting the ROM due to an in-movie restart, this needs to be false. \param
 * wait Whether the calling thread will wait for other core operations to
 * complete. When true, the Busy result is never returned. \return The operation
 * result
 */
CoreResult vr_reset_rom(bool reset_save_data = false, bool stop_vcr = true, bool wait = false);

/**
 * \brief Toggles between fullscreen and windowed mode
 */
void toggle_fullscreen_mode();

/**
 * \brief Gets the fullscreen state
 */
bool vr_is_fullscreen();

/**
 * \brief Gets the GS button state
 */
bool get_gs_button();

/**
 * \brief Sets the GS button state
 */
void set_gs_button(bool);

/**
 * \brief Gets the path to the save directory
 */
std::filesystem::path get_saves_directory();

void pure_interpreter();
void compare_core();
extern void jump_to_func();
void update_count();
int32_t check_cop1_unusable();
void terminate_emu();

#define jump_to(a)                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
    {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
        jump_to_address = a;                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
        jump_to_func();                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
    }

// Mask all exceptions, and set precision to 53 bits
#define TRUNC_MODE 0xE3F
#define ROUND_MODE 0x23F
#define CEIL_MODE 0xA3F
#define FLOOR_MODE 0x63F

#define VR_SECTION_RSP 0
#define VR_SECTION_TIMER 1
#define VR_SECTION_LUA_ATINTERVAL 2
#define VR_SECTION_LUA_ATVI 3

#ifdef VR_PROFILE
void start_section(int32_t section_type);
void end_section(int32_t section_type);
#else
#define start_section(x)
#define end_section(x)
#endif
