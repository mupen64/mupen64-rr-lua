/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <memory/memory.h>
#include <memory/pif.h>
#include <memory/savestates.h>
#include <r4300/exception.h>
#include <r4300/interrupt.h>
#include <r4300/macros.h>
#include <r4300/ops.h>
#include <r4300/r4300.h>
#include <r4300/recomp.h>
#include <r4300/timers.h>
#include <r4300/vcr.h>

#ifdef WIN32
// VirtualAlloc
#include <Windows.h>
#endif

std::thread emu_thread_handle;
std::thread audio_thread_handle;

std::atomic<bool> audio_thread_stop_requested;

// Lock to prevent emu state change race conditions
std::recursive_mutex g_emu_cs;

std::filesystem::path rom_path;

bool g_vr_beq_ignore_jmp;
volatile bool emu_launched = false;
volatile bool emu_paused = false;
volatile bool core_executing = false;
volatile bool emu_resetting = false;
std::atomic<size_t> frame_advance_outstanding = 0;
size_t g_total_frames = 0;
bool fullscreen = false;
bool gs_button = false;

uint32_t i, dynacore = 0, interpcore = 0;
int32_t stop, llbit;
int64_t reg[32], hi, lo;
int64_t local_rs, local_rt;
uint32_t reg_cop0[32];
int32_t local_rs32, local_rt32;
uint32_t jump_target;
float* reg_cop1_simple[32];
double* reg_cop1_double[32];
int32_t reg_cop1_fgr_32[32];
int64_t reg_cop1_fgr_64[32];
int32_t FCR0, FCR31;
tlb tlb_e[32];
uint32_t delay_slot, skip_jump = 0, dyna_interp = 0, last_addr;
uint64_t debug_count = 0;
uint32_t next_interrupt, CIC_Chip;
precomp_instr* PC;
char invalid_code[0x100000];
std::atomic<bool> screen_invalidated = true;
precomp_block *blocks[0x100000], *actual;
int32_t rounding_mode = MUP_ROUND_NEAREST;
int32_t trunc_mode = MUP_ROUND_TRUNC, round_mode = MUP_ROUND_NEAREST, ceil_mode = MUP_ROUND_CEIL, floor_mode = MUP_ROUND_FLOOR;
int16_t x87_status_word;
void (*code)();
uint32_t next_vi;
int32_t vi_field = 0;
bool g_vr_fast_forward;
bool g_vr_frame_skipped;
core_system_type g_sys_type;

FILE* g_eeprom_file;
FILE* g_sram_file;
FILE* g_fram_file;
FILE* g_mpak_file;

/*#define check_memory() \
   if (!invalid_code[address>>12]) \
       invalid_code[address>>12] = 1;*/

#define check_memory()                                                              \
    if (!invalid_code[address >> 12])                                               \
        if (blocks[address >> 12]->block[(address & 0xFFF) / 4].ops != NOTCOMPILED) \
            invalid_code[address >> 12] = 1;

void core_vr_invalidate_visuals()
{
    screen_invalidated = true;
}

std::filesystem::path get_sram_path()
{
    return std::format(L"{}{} {}.sra", g_core->get_saves_directory().wstring(), string_to_wstring((const char*)ROM_HEADER.nom), core_vr_country_code_to_country_name(ROM_HEADER.Country_code));
}

std::filesystem::path get_eeprom_path()
{
    return std::format(L"{}{} {}.eep", g_core->get_saves_directory().wstring(), string_to_wstring((const char*)ROM_HEADER.nom), core_vr_country_code_to_country_name(ROM_HEADER.Country_code));
}

std::filesystem::path get_flashram_path()
{
    return std::format(L"{}{} {}.fla", g_core->get_saves_directory().wstring(), string_to_wstring((const char*)ROM_HEADER.nom), core_vr_country_code_to_country_name(ROM_HEADER.Country_code));
}

std::filesystem::path get_mempak_path()
{
    return std::format(L"{}{} {}.mpk", g_core->get_saves_directory().wstring(), string_to_wstring((const char*)ROM_HEADER.nom), core_vr_country_code_to_country_name(ROM_HEADER.Country_code));
}

void core_vr_resume_emu()
{
    if (emu_launched)
    {
        emu_paused = 0;
    }

    g_core->callbacks.emu_paused_changed(emu_paused);
}


void core_vr_pause_emu()
{
    if (!vcr_allows_core_pause())
    {
        return;
    }

    if (emu_launched)
    {
        emu_paused = 1;
    }

    g_core->callbacks.emu_paused_changed(emu_paused);
}

void core_vr_frame_advance(size_t count)
{
    frame_advance_outstanding = count;
    core_vr_resume_emu();
}

bool core_vr_get_paused()
{
    return emu_paused;
}

bool core_vr_get_frame_advance()
{
    return frame_advance_outstanding;
}

void terminate_emu()
{
    stop = 1;
}

bool core_vr_get_core_executing()
{
    return core_executing;
}

bool core_vr_get_launched()
{
    return emu_launched;
}

std::filesystem::path CALL core_vr_get_rom_path()
{
    return rom_path;
}

void NI()
{
    g_core->log_error(std::format(L"NI() @ {:#06x}\n", (int32_t)PC->addr));
    g_core->log_error(L"opcode not implemented : ");
    if (PC->addr >= 0xa4000000 && PC->addr < 0xa4001000)
        g_core->log_info(std::format(L"{:#06x}:{:#06x}", (int32_t)PC->addr, (int32_t)SP_DMEM[(PC->addr - 0xa4000000) / 4]));
    else
        g_core->log_info(std::format(L"{:#06x}:{:#06x}", (int32_t)PC->addr, (int32_t)rdram[(PC->addr - 0x80000000) / 4]));
    stop = 1;
}

void RESERVED()
{
    g_core->log_info(L"reserved opcode : ");
    if (PC->addr >= 0xa4000000 && PC->addr < 0xa4001000)
        g_core->log_info(std::format(L"{:#06x}:{:#06x}", (int32_t)PC->addr, (int32_t)SP_DMEM[(PC->addr - 0xa4000000) / 4]));
    else
        g_core->log_info(std::format(L"{:#06x}:{:#06x}", (int32_t)PC->addr, (int32_t)rdram[(PC->addr - 0x80000000) / 4]));
    stop = 1;
}

void FIN_BLOCK()
{
    if (!delay_slot)
    {
        jump_to((PC - 1)->addr + 4);
        PC->ops();
        if (dynacore)
            dyna_jump();
    }
    else
    {
        precomp_block* blk = actual;
        precomp_instr* inst = PC;
        jump_to((PC - 1)->addr + 4);

        if (!skip_jump)
        {
            PC->ops();
            actual = blk;
            PC = inst + 1;
        }
        else
            PC->ops();

        if (dynacore)
            dyna_jump();
    }
}

void J()
{
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump)
        PC = actual->block +
        (((((PC - 2)->f.j.inst_index << 2) | ((PC - 1)->addr & 0xF0000000)) - actual->start) >> 2);
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void J_OUT()
{
    jump_target = (PC->addr & 0xF0000000) | (PC->f.j.inst_index << 2);
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump)
        jump_to(jump_target);
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void J_IDLE()
{
    int32_t skip;
    update_count();
    skip = next_interrupt - core_Count;
    if (skip > 3)
        core_Count += (skip & 0xFFFFFFFC);
    else
        J();
}

void JAL()
{
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump)
    {
        reg[31] = PC->addr;
        sign_extended(reg[31]);

        PC = actual->block +
        (((((PC - 2)->f.j.inst_index << 2) | ((PC - 1)->addr & 0xF0000000)) - actual->start) >> 2);
    }
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void JAL_OUT()
{
    jump_target = (PC->addr & 0xF0000000) | (PC->f.j.inst_index << 2);
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump)
    {
        reg[31] = PC->addr;
        sign_extended(reg[31]);

        jump_to(jump_target);
    }
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void JAL_IDLE()
{
    int32_t skip;
    update_count();
    skip = next_interrupt - core_Count;
    if (skip > 3)
        core_Count += (skip & 0xFFFFFFFC);
    else
        JAL();
}

void BEQ()
{
    local_rs = core_irs;
    local_rt = core_irt;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (local_rs == local_rt && !skip_jump && !g_vr_beq_ignore_jmp)
        PC += (PC - 2)->f.i.immediate - 1;
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BEQ_OUT()
{
    local_rs = core_irs;
    local_rt = core_irt;
    jump_target = (int32_t)PC->f.i.immediate;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump && local_rs == local_rt)
        jump_to(PC->addr + ((jump_target - 1) << 2));
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BEQ_IDLE()
{
    int32_t skip;
    if (core_irs == core_irt)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BEQ();
    }
    else
        BEQ();
}

void BNE()
{
    local_rs = core_irs;
    local_rt = core_irt;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (local_rs != local_rt && !skip_jump)
        PC += (PC - 2)->f.i.immediate - 1;
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BNE_OUT()
{
    local_rs = core_irs;
    local_rt = core_irt;
    jump_target = (int32_t)PC->f.i.immediate;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump && local_rs != local_rt)
        jump_to(PC->addr + ((jump_target - 1) << 2));
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BNE_IDLE()
{
    int32_t skip;
    if (core_irs != core_irt)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BNE();
    }
    else
        BNE();
}

void BLEZ()
{
    local_rs = core_irs;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (local_rs <= 0 && !skip_jump)
        PC += (PC - 2)->f.i.immediate - 1;
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BLEZ_OUT()
{
    local_rs = core_irs;
    jump_target = (int32_t)PC->f.i.immediate;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump && local_rs <= 0)
        jump_to(PC->addr + ((jump_target - 1) << 2));
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BLEZ_IDLE()
{
    int32_t skip;
    if (core_irs <= core_irt)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BLEZ();
    }
    else
        BLEZ();
}

void BGTZ()
{
    local_rs = core_irs;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (local_rs > 0 && !skip_jump)
        PC += (PC - 2)->f.i.immediate - 1;
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BGTZ_OUT()
{
    local_rs = core_irs;
    jump_target = (int32_t)PC->f.i.immediate;
    PC++;
    delay_slot = 1;
    PC->ops();
    update_count();
    delay_slot = 0;
    if (!skip_jump && local_rs > 0)
        jump_to(PC->addr + ((jump_target - 1) << 2));
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BGTZ_IDLE()
{
    int32_t skip;
    if (core_irs > core_irt)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BGTZ();
    }
    else
        BGTZ();
}

void ADDI()
{
    irt32 = irs32 + core_iimmediate;
    sign_extended(core_irt);
    PC++;
}

void ADDIU()
{
    irt32 = irs32 + core_iimmediate;
    sign_extended(core_irt);
    PC++;
}

void SLTI()
{
    if (core_irs < core_iimmediate)
        core_irt = 1;
    else
        core_irt = 0;
    PC++;
}

void SLTIU()
{
    if ((uint64_t)core_irs < (uint64_t)((int64_t)core_iimmediate))
        core_irt = 1;
    else
        core_irt = 0;
    PC++;
}

void ANDI()
{
    core_irt = core_irs & (uint16_t)core_iimmediate;
    PC++;
}

void ORI()
{
    core_irt = core_irs | (uint16_t)core_iimmediate;
    PC++;
}

void XORI()
{
    core_irt = core_irs ^ (uint16_t)core_iimmediate;
    PC++;
}

void LUI()
{
    irt32 = core_iimmediate << 16;
    sign_extended(core_irt);
    PC++;
}

void BEQL()
{
    if (core_irs == core_irt)
    {
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump)
            PC += (PC - 2)->f.i.immediate - 1;
    }
    else
    {
        PC += 2;
        update_count();
    }
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BEQL_OUT()
{
    if (core_irs == core_irt)
    {
        jump_target = (int32_t)PC->f.i.immediate;
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump)
            jump_to(PC->addr + ((jump_target - 1) << 2));
    }
    else
    {
        PC += 2;
        update_count();
    }
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BEQL_IDLE()
{
    int32_t skip;
    if (core_irs == core_irt)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BEQL();
    }
    else
        BEQL();
}

void BNEL()
{
    if (core_irs != core_irt)
    {
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump)
            PC += (PC - 2)->f.i.immediate - 1;
    }
    else
    {
        PC += 2;
        update_count();
    }
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BNEL_OUT()
{
    if (core_irs != core_irt)
    {
        jump_target = (int32_t)PC->f.i.immediate;
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump)
            jump_to(PC->addr + ((jump_target - 1) << 2));
    }
    else
    {
        PC += 2;
        update_count();
    }
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BNEL_IDLE()
{
    int32_t skip;
    if (core_irs != core_irt)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BNEL();
    }
    else
        BNEL();
}

void BLEZL()
{
    if (core_irs <= 0)
    {
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump)
            PC += (PC - 2)->f.i.immediate - 1;
    }
    else
    {
        PC += 2;
        update_count();
    }
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BLEZL_OUT()
{
    if (core_irs <= 0)
    {
        jump_target = (int32_t)PC->f.i.immediate;
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump)
            jump_to(PC->addr + ((jump_target - 1) << 2));
    }
    else
    {
        PC += 2;
        update_count();
    }
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BLEZL_IDLE()
{
    int32_t skip;
    if (core_irs <= core_irt)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BLEZL();
    }
    else
        BLEZL();
}

void BGTZL()
{
    if (core_irs > 0)
    {
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump)
            PC += (PC - 2)->f.i.immediate - 1;
    }
    else
    {
        PC += 2;
        update_count();
    }
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BGTZL_OUT()
{
    if (core_irs > 0)
    {
        jump_target = (int32_t)PC->f.i.immediate;
        PC++;
        delay_slot = 1;
        PC->ops();
        update_count();
        delay_slot = 0;
        if (!skip_jump)
            jump_to(PC->addr + ((jump_target - 1) << 2));
    }
    else
    {
        PC += 2;
        update_count();
    }
    last_addr = PC->addr;
    if (next_interrupt <= core_Count)
        gen_interrupt();
}

void BGTZL_IDLE()
{
    int32_t skip;
    if (core_irs > core_irt)
    {
        update_count();
        skip = next_interrupt - core_Count;
        if (skip > 3)
            core_Count += (skip & 0xFFFFFFFC);
        else
            BGTZL();
    }
    else
        BGTZL();
}

void DADDI()
{
    core_irt = core_irs + core_iimmediate;
    PC++;
}

void DADDIU()
{
    core_irt = core_irs + core_iimmediate;
    PC++;
}

void LDL()
{
    uint64_t word = 0;
    PC++;
    switch ((core_lsaddr) & 7)
    {
    case 0:
        address = core_lsaddr;
        rdword = (uint64_t*)&core_lsrt;
        read_dword_in_memory();
        break;
    case 1:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFF) | (word << 8);
        break;
    case 2:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFF) | (word << 16);
        break;
    case 3:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFF) | (word << 24);
        break;
    case 4:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFFFF) | (word << 32);
        break;
    case 5:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFFFFFFLL) | (word << 40);
        break;
    case 6:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFFFFFFFFLL) | (word << 48);
        break;
    case 7:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFFFFFFFFFFLL) | (word << 56);
        break;
    }
}

void LDR()
{
    uint64_t word = 0;
    PC++;
    switch ((core_lsaddr) & 7)
    {
    case 0:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFFFFFFFFFF00LL) | (word >> 56);
        break;
    case 1:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFFFFFFFF0000LL) | (word >> 48);
        break;
    case 2:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFFFFFF000000LL) | (word >> 40);
        break;
    case 3:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFFFF00000000LL) | (word >> 32);
        break;
    case 4:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFF0000000000LL) | (word >> 24);
        break;
    case 5:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFF000000000000LL) | (word >> 16);
        break;
    case 6:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &word;
        read_dword_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFF00000000000000LL) | (word >> 8);
        break;
    case 7:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = (uint64_t*)&core_lsrt;
        read_dword_in_memory();
        break;
    }
}

void LB()
{
    PC++;
    address = core_lsaddr;
    rdword = (uint64_t*)&core_lsrt;
    read_byte_in_memory();
    if (address)
        sign_extendedb(core_lsrt);
}

void LH()
{
    PC++;
    address = core_lsaddr;
    rdword = (uint64_t*)&core_lsrt;
    read_hword_in_memory();
    if (address)
        sign_extendedh(core_lsrt);
}

void LWL()
{
    uint64_t word = 0;
    PC++;
    switch ((core_lsaddr) & 3)
    {
    case 0:
        address = core_lsaddr;
        rdword = (uint64_t*)&core_lsrt;
        read_word_in_memory();
        break;
    case 1:
        address = (core_lsaddr) & 0xFFFFFFFC;
        rdword = &word;
        read_word_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFF) | (word << 8);
        break;
    case 2:
        address = (core_lsaddr) & 0xFFFFFFFC;
        rdword = &word;
        read_word_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFF) | (word << 16);
        break;
    case 3:
        address = (core_lsaddr) & 0xFFFFFFFC;
        rdword = &word;
        read_word_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFF) | (word << 24);
        break;
    }
    if (address)
        sign_extended(core_lsrt);
}

void LW()
{
    PC++;
    address = core_lsaddr;
    rdword = (uint64_t*)&core_lsrt;
    read_word_in_memory();
    if (address)
        sign_extended(core_lsrt);
}

void LBU()
{
    PC++;
    address = core_lsaddr;
    rdword = (uint64_t*)&core_lsrt;
    read_byte_in_memory();
}

void LHU()
{
    PC++;
    address = core_lsaddr;
    rdword = (uint64_t*)&core_lsrt;
    read_hword_in_memory();
}

void LWR()
{
    uint64_t word = 0;
    PC++;
    switch ((core_lsaddr) & 3)
    {
    case 0:
        address = (core_lsaddr) & 0xFFFFFFFC;
        rdword = &word;
        read_word_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFFFFFFFFFF00LL) | ((word >> 24) & 0xFF);
        break;
    case 1:
        address = (core_lsaddr) & 0xFFFFFFFC;
        rdword = &word;
        read_word_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFFFFFFFF0000LL) | ((word >> 16) & 0xFFFF);
        break;
    case 2:
        address = (core_lsaddr) & 0xFFFFFFFC;
        rdword = &word;
        read_word_in_memory();
        if (address)
            core_lsrt = (core_lsrt & 0xFFFFFFFFFF000000LL) | ((word >> 8) & 0XFFFFFF);
        break;
    case 3:
        address = (core_lsaddr) & 0xFFFFFFFC;
        rdword = (uint64_t*)&core_lsrt;
        read_word_in_memory();
        if (address)
            sign_extended(core_lsrt);
    }
}

void LWU()
{
    PC++;
    address = core_lsaddr;
    rdword = (uint64_t*)&core_lsrt;
    read_word_in_memory();
}

void SB()
{
    PC++;
    address = core_lsaddr;
    g_byte = (unsigned char)(core_lsrt & 0xFF);
    write_byte_in_memory();
    check_memory();
}

void SH()
{
    PC++;
    address = core_lsaddr;
    hword = (uint16_t)(core_lsrt & 0xFFFF);
    write_hword_in_memory();
    check_memory();
}

void SWL()
{
    uint64_t old_word = 0;
    PC++;
    switch ((core_lsaddr) & 3)
    {
    case 0:
        address = (core_lsaddr) & 0xFFFFFFFC;
        word = (uint32_t)core_lsrt;
        write_word_in_memory();
        check_memory();
        break;
    case 1:
        address = (core_lsaddr) & 0xFFFFFFFC;
        rdword = &old_word;
        read_word_in_memory();
        if (address)
        {
            word = ((uint32_t)core_lsrt >> 8) | (old_word & 0xFF000000);
            write_word_in_memory();
            check_memory();
        }
        break;
    case 2:
        address = (core_lsaddr) & 0xFFFFFFFC;
        rdword = &old_word;
        read_word_in_memory();
        if (address)
        {
            word = ((uint32_t)core_lsrt >> 16) | (old_word & 0xFFFF0000);
            write_word_in_memory();
            check_memory();
        }
        break;
    case 3:
        address = core_lsaddr;
        g_byte = (unsigned char)(core_lsrt >> 24);
        write_byte_in_memory();
        check_memory();
        break;
    }
}

void SW()
{
    PC++;
    address = core_lsaddr;
    word = (uint32_t)(core_lsrt & 0xFFFFFFFF);
    write_word_in_memory();
    check_memory();
}

void SDL()
{
    uint64_t old_word = 0;
    PC++;
    switch ((core_lsaddr) & 7)
    {
    case 0:
        address = (core_lsaddr) & 0xFFFFFFF8;
        dword = core_lsrt;
        write_dword_in_memory();
        check_memory();
        break;
    case 1:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = ((uint64_t)core_lsrt >> 8) | (old_word & 0xFF00000000000000LL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    case 2:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = ((uint64_t)core_lsrt >> 16) | (old_word & 0xFFFF000000000000LL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    case 3:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = ((uint64_t)core_lsrt >> 24) | (old_word & 0xFFFFFF0000000000LL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    case 4:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = ((uint64_t)core_lsrt >> 32) | (old_word & 0xFFFFFFFF00000000LL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    case 5:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = ((uint64_t)core_lsrt >> 40) | (old_word & 0xFFFFFFFFFF000000LL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    case 6:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = ((uint64_t)core_lsrt >> 48) | (old_word & 0xFFFFFFFFFFFF0000LL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    case 7:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = ((uint64_t)core_lsrt >> 56) | (old_word & 0xFFFFFFFFFFFFFF00LL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    }
}

void SDR()
{
    uint64_t old_word = 0;
    PC++;
    switch ((core_lsaddr) & 7)
    {
    case 0:
        address = core_lsaddr;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = (core_lsrt << 56) | (old_word & 0x00FFFFFFFFFFFFFFLL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    case 1:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = (core_lsrt << 48) | (old_word & 0x0000FFFFFFFFFFFFLL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    case 2:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = (core_lsrt << 40) | (old_word & 0x000000FFFFFFFFFFLL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    case 3:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = (core_lsrt << 32) | (old_word & 0x00000000FFFFFFFFLL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    case 4:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = (core_lsrt << 24) | (old_word & 0x0000000000FFFFFFLL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    case 5:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = (core_lsrt << 16) | (old_word & 0x000000000000FFFFLL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    case 6:
        address = (core_lsaddr) & 0xFFFFFFF8;
        rdword = &old_word;
        read_dword_in_memory();
        if (address)
        {
            dword = (core_lsrt << 8) | (old_word & 0x00000000000000FFLL);
            write_dword_in_memory();
            check_memory();
        }
        break;
    case 7:
        address = (core_lsaddr) & 0xFFFFFFF8;
        dword = core_lsrt;
        write_dword_in_memory();
        check_memory();
        break;
    }
}

void SWR()
{
    uint64_t old_word = 0;
    PC++;
    switch ((core_lsaddr) & 3)
    {
    case 0:
        address = core_lsaddr;
        rdword = &old_word;
        read_word_in_memory();
        if (address)
        {
            word = ((uint32_t)core_lsrt << 24) | (old_word & 0x00FFFFFF);
            write_word_in_memory();
            check_memory();
        }
        break;
    case 1:
        address = (core_lsaddr) & 0xFFFFFFFC;
        rdword = &old_word;
        read_word_in_memory();
        if (address)
        {
            word = ((uint32_t)core_lsrt << 16) | (old_word & 0x0000FFFF);
            write_word_in_memory();
            check_memory();
        }
        break;
    case 2:
        address = (core_lsaddr) & 0xFFFFFFFC;
        rdword = &old_word;
        read_word_in_memory();
        if (address)
        {
            word = ((uint32_t)core_lsrt << 8) | (old_word & 0x000000FF);
            write_word_in_memory();
            check_memory();
        }
        break;
    case 3:
        address = (core_lsaddr) & 0xFFFFFFFC;
        word = (uint32_t)core_lsrt;
        write_word_in_memory();
        check_memory();
        break;
    }
}

void CACHE()
{
    PC++;
}

void LL()
{
    PC++;
    address = core_lsaddr;
    rdword = (uint64_t*)&core_lsrt;
    read_word_in_memory();
    if (address)
    {
        sign_extended(core_lsrt);
        llbit = 1;
    }
}

void LWC1()
{
    uint64_t temp;
    if (check_cop1_unusable())
        return;
    PC++;
    address = core_lslfaddr;
    rdword = &temp;
    read_word_in_memory();
    if (address)
        *((int32_t*)reg_cop1_simple[core_lslfft]) = *rdword;
}

void LDC1()
{
    if (check_cop1_unusable())
        return;
    PC++;
    address = core_lslfaddr;
    rdword = (uint64_t*)reg_cop1_double[core_lslfft];
    read_dword_in_memory();
}

void LD()
{
    PC++;
    address = core_lsaddr;
    rdword = (uint64_t*)&core_lsrt;
    read_dword_in_memory();
}

void SC()
{
    /*PC++;
    g_core->log_info(L"SC");
    if (llbit) {
       address = lsaddr;
       word = (uint32_t)(core_lsrt & 0xFFFFFFFF);
       write_word_in_memory();
    }
    core_lsrt = llbit;*/

    PC++;
    if (llbit)
    {
        address = core_lsaddr;
        word = (uint32_t)(core_lsrt & 0xFFFFFFFF);
        write_word_in_memory();
        check_memory();
        llbit = 0;
        core_lsrt = 1;
    }
    else
    {
        core_lsrt = 0;
    }
}

void SWC1()
{
    if (check_cop1_unusable())
        return;
    PC++;
    address = core_lslfaddr;
    word = *((int32_t*)reg_cop1_simple[core_lslfft]);
    write_word_in_memory();
    check_memory();
}

void SDC1()
{
    if (check_cop1_unusable())
        return;
    PC++;
    address = core_lslfaddr;
    dword = *((uint64_t*)reg_cop1_double[core_lslfft]);
    write_dword_in_memory();
    check_memory();
}

void SD()
{
    PC++;
    address = core_lsaddr;
    dword = core_lsrt;
    write_dword_in_memory();
    check_memory();
}

void NOTCOMPILED()
{
    if ((PC->addr >> 16) == 0xa400)
        recompile_block((int32_t*)SP_DMEM, blocks[0xa4000000 >> 12], PC->addr);
    else
    {
        uint32_t paddr = 0;
        if (PC->addr >= 0x80000000 && PC->addr < 0xc0000000)
            paddr = PC->addr;
        // else paddr = (tlb_LUT_r[PC->addr>>12]&0xFFFFF000)|(PC->addr&0xFFF);
        else
            paddr = virtual_to_physical_address(PC->addr, 2);
        if (paddr)
        {
            if ((paddr & 0x1FFFFFFF) >= 0x10000000)
            {
                // g_core->log_info(L"not compiled rom:{:#06x}", paddr);
                recompile_block(
                (int32_t*)rom + ((((paddr - (PC->addr - blocks[PC->addr >> 12]->start)) & 0x1FFFFFFF) - 0x10000000) >> 2),
                blocks[PC->addr >> 12], PC->addr);
            }
            else
                recompile_block(
                (int32_t*)(rdram + (((paddr - (PC->addr - blocks[PC->addr >> 12]->start)) & 0x1FFFFFFF) >> 2)),
                blocks[PC->addr >> 12], PC->addr);
        }
        else
            g_core->log_info(L"not compiled exception");
    }
    PC->ops();
    if (dynacore)
        dyna_jump();
    //*return_address = (uint32_t)(blocks[PC->addr>>12]->code + PC->local_addr);
    // else
    // PC->ops();
}

void NOTCOMPILED2()
{
    NOTCOMPILED();
}

static inline uint32_t update_invalid_addr(uint32_t addr)
{
    if (addr >= 0x80000000 && addr < 0xa0000000)
    {
        if (invalid_code[addr >> 12])
            invalid_code[(addr + 0x20000000) >> 12] = 1;
        if (invalid_code[(addr + 0x20000000) >> 12])
            invalid_code[addr >> 12] = 1;
        return addr;
    }
    else if (addr >= 0xa0000000 && addr < 0xc0000000)
    {
        if (invalid_code[addr >> 12])
            invalid_code[(addr - 0x20000000) >> 12] = 1;
        if (invalid_code[(addr - 0x20000000) >> 12])
            invalid_code[addr >> 12] = 1;
        return addr;
    }
    else
    {
        uint32_t paddr = virtual_to_physical_address(addr, 2);
        if (paddr)
        {
            uint32_t beg_paddr = paddr - (addr - (addr & ~0xFFF));
            update_invalid_addr(paddr);
            if (invalid_code[(beg_paddr + 0x000) >> 12])
                invalid_code[addr >> 12] = 1;
            if (invalid_code[(beg_paddr + 0xFFC) >> 12])
                invalid_code[addr >> 12] = 1;
            if (invalid_code[addr >> 12])
                invalid_code[(beg_paddr + 0x000) >> 12] = 1;
            if (invalid_code[addr >> 12])
                invalid_code[(beg_paddr + 0xFFC) >> 12] = 1;
        }
        return paddr;
    }
}

#define addr jump_to_address
uint32_t jump_to_address;

inline void jump_to_func()
{
    // #ifdef _DEBUG
    //	g_core->log_info(L"dyna jump: {:#08x}", addr);
    // #endif
    uint32_t paddr;
    if (skip_jump)
        return;
    paddr = update_invalid_addr(addr);
    if (!paddr)
        return;
    actual = blocks[addr >> 12];
    if (invalid_code[addr >> 12])
    {
        if (!blocks[addr >> 12])
        {
            blocks[addr >> 12] = (precomp_block*)malloc(sizeof(precomp_block));
            actual = blocks[addr >> 12];
            blocks[addr >> 12]->code = NULL;
            blocks[addr >> 12]->block = NULL;
            blocks[addr >> 12]->jumps_table = NULL;
        }
        blocks[addr >> 12]->start = addr & ~0xFFF;
        blocks[addr >> 12]->end = (addr & ~0xFFF) + 0x1000;
        init_block((int32_t*)(rdram + (((paddr - (addr - blocks[addr >> 12]->start)) & 0x1FFFFFFF) >> 2)),
                   blocks[addr >> 12]);
    }
    PC = actual->block + ((addr - actual->start) >> 2);

    if (dynacore)
        dyna_jump();
}
#undef addr

int32_t check_cop1_unusable()
{
    if (!(core_Status & 0x20000000))
    {
        core_Cause = (11 << 2) | 0x10000000;
        exception_general();
        return 1;
    }
    return 0;
}

void update_count()
{
    if (interpcore)
    {
        core_Count = core_Count + (interp_addr - last_addr) / 2;
        last_addr = interp_addr;
    }
    else
    {
        if (PC->addr < last_addr)
        {
            g_core->log_info(L"PC->addr < last_addr");
        }
        core_Count = core_Count + (PC->addr - last_addr) / 2;
        last_addr = PC->addr;
    }
}

void init_blocks()
{
    int32_t i;
    for (i = 0; i < 0x100000; i++)
    {
        invalid_code[i] = 1;
        blocks[i] = NULL;
    }
    blocks[0xa4000000 >> 12] = (precomp_block*)malloc(sizeof(precomp_block));
    invalid_code[0xa4000000 >> 12] = 1;
    blocks[0xa4000000 >> 12]->code = NULL;
    blocks[0xa4000000 >> 12]->block = NULL;
    blocks[0xa4000000 >> 12]->jumps_table = NULL;
    blocks[0xa4000000 >> 12]->start = 0xa4000000;
    blocks[0xa4000000 >> 12]->end = 0xa4001000;
    actual = blocks[0xa4000000 >> 12];
    init_block((int32_t*)SP_DMEM, blocks[0xa4000000 >> 12]);
    PC = actual->block + (0x40 / 4);
}


void print_stop_debug()
{
    g_core->log_info(std::format(L"PC={:#08x}:{:#08x}", PC->addr, rdram[(PC->addr & 0xFFFFFF) / 4]));
    for (int32_t j = 0; j < 16; j++)
        g_core->log_info(std::format(L"reg[{}]:{:#08x}{:#08x}        reg[{}]:{:#08x}{:#08x}", j, (uint32_t)(reg[j] >> 32), (uint32_t)reg[j], j + 16, (uint32_t)(reg[j + 16] >> 32), (uint32_t)reg[j + 16]));
    g_core->log_info(std::format(L"hi:{:#08x}{:#08x}        lo:{:#08x}{:#08x}",
                                 (uint32_t)(hi >> 32),
                                 (uint32_t)hi,
                                 (uint32_t)(lo >> 32),
                                 (uint32_t)lo));
    g_core->log_info(std::format(L"Executed {} ({:#08x}) instructions", debug_count, debug_count));
}

void core_start()
{
    int64_t CRC = 0;
    uint32_t j;

    j = 0;
    debug_count = 0;
    g_core->log_info(L"demarrage r4300");
    memcpy((char*)SP_DMEM + 0x40, rom + 0x40, 0xFBC);
    delay_slot = 0;
    stop = 0;
    for (i = 0; i < 32; i++)
    {
        reg[i] = 0;
        reg_cop0[i] = 0;
        reg_cop1_fgr_32[i] = 0;
        reg_cop1_fgr_64[i] = 0;

        reg_cop1_double[i] = (double*)&reg_cop1_fgr_64[i];
        reg_cop1_simple[i] = (float*)&reg_cop1_fgr_64[i];

        // --------------tlb------------------------
        tlb_e[i].mask = 0;
        tlb_e[i].vpn2 = 0;
        tlb_e[i].g = 0;
        tlb_e[i].asid = 0;
        tlb_e[i].pfn_even = 0;
        tlb_e[i].c_even = 0;
        tlb_e[i].d_even = 0;
        tlb_e[i].v_even = 0;
        tlb_e[i].pfn_odd = 0;
        tlb_e[i].c_odd = 0;
        tlb_e[i].d_odd = 0;
        tlb_e[i].v_odd = 0;
        tlb_e[i].r = 0;
        // tlb_e[i].check_parity_mask=0x1000;

        tlb_e[i].start_even = 0;
        tlb_e[i].end_even = 0;
        tlb_e[i].phys_even = 0;
        tlb_e[i].start_odd = 0;
        tlb_e[i].end_odd = 0;
        tlb_e[i].phys_odd = 0;
    }
    memset(tlb_LUT_r, 0, sizeof(tlb_LUT_r));
    memset(tlb_LUT_r, 0, sizeof(tlb_LUT_w));
    llbit = 0;
    hi = 0;
    lo = 0;
    FCR0 = 0x511;
    FCR31 = 0;

    //--------
    /*reg[20]=1;
    reg[22]=0x3F;
    reg[29]=0xFFFFFFFFA0400000LL;
    Random=31;
    Status=0x70400004;
    Config=0x66463;
    PRevID=0xb00;*/
    //--------

    // the following values are extracted from the pj64 source code
    // thanks to Zilmar and Jabo

    reg[6] = 0xFFFFFFFFA4001F0CLL;
    reg[7] = 0xFFFFFFFFA4001F08LL;
    reg[8] = 0x00000000000000C0LL;
    reg[10] = 0x0000000000000040LL;
    reg[11] = 0xFFFFFFFFA4000040LL;
    reg[29] = 0xFFFFFFFFA4001FF0LL;

    core_Random = 31;
    core_Status = 0x34000000;
    core_Config_cop0 = 0x6e463;
    core_PRevID = 0xb00;
    core_Count = 0x5000;
    core_Cause = 0x5C;
    core_Context = 0x7FFFF0;
    core_EPC = 0xFFFFFFFF;
    core_BadVAddr = 0xFFFFFFFF;
    core_ErrorEPC = 0xFFFFFFFF;

    for (i = 0x40 / 4; i < (0x1000 / 4); i++)
        CRC += SP_DMEM[i];
    switch (CRC)
    {
    case 0x000000D0027FDF31LL:
    case 0x000000CFFB631223LL:
        CIC_Chip = 1;
        break;
    case 0x000000D057C85244LL:
        CIC_Chip = 2;
        break;
    case 0x000000D6497E414BLL:
        CIC_Chip = 3;
        break;
    case 0x0000011A49F60E96LL:
        CIC_Chip = 5;
        break;
    case 0x000000D6D5BE5580LL:
        CIC_Chip = 6;
        break;
    default:
        CIC_Chip = 2;
    }

    switch (ROM_HEADER.Country_code & 0xFF)
    {
    case 0x44:
    case 0x46:
    case 0x49:
    case 0x50:
    case 0x53:
    case 0x55:
    case 0x58:
    case 0x59:
        switch (CIC_Chip)
        {
        case 2:
            reg[5] = 0xFFFFFFFFC0F1D859LL;
            reg[14] = 0x000000002DE108EALL;
            break;
        case 3:
            reg[5] = 0xFFFFFFFFD4646273LL;
            reg[14] = 0x000000001AF99984LL;
            break;
        case 5:
            SP_IMEM[1] = 0xBDA807FC;
            reg[5] = 0xFFFFFFFFDECAAAD1LL;
            reg[14] = 0x000000000CF85C13LL;
            reg[24] = 0x0000000000000002LL;
            break;
        case 6:
            reg[5] = 0xFFFFFFFFB04DC903LL;
            reg[14] = 0x000000001AF99984LL;
            reg[24] = 0x0000000000000002LL;
            break;
        }
        reg[23] = 0x0000000000000006LL;
        reg[31] = 0xFFFFFFFFA4001554LL;
        break;
    case 0x37:
    case 0x41:
    case 0x45:
    case 0x4A:
    default:
        switch (CIC_Chip)
        {
        case 2:
            reg[5] = 0xFFFFFFFFC95973D5LL;
            reg[14] = 0x000000002449A366LL;
            break;
        case 3:
            reg[5] = 0xFFFFFFFF95315A28LL;
            reg[14] = 0x000000005BACA1DFLL;
            break;
        case 5:
            SP_IMEM[1] = 0x8DA807FC;
            reg[5] = 0x000000005493FB9ALL;
            reg[14] = 0xFFFFFFFFC2C20384LL;
            break;
        case 6:
            reg[5] = 0xFFFFFFFFE067221FLL;
            reg[14] = 0x000000005CD2B70FLL;
            break;
        }
        reg[20] = 0x0000000000000001LL;
        reg[24] = 0x0000000000000003LL;
        reg[31] = 0xFFFFFFFFA4001550LL;
    }
    switch (CIC_Chip)
    {
    case 1:
        reg[22] = 0x000000000000003FLL;
        break;
    case 2:
        reg[1] = 0x0000000000000001LL;
        reg[2] = 0x000000000EBDA536LL;
        reg[3] = 0x000000000EBDA536LL;
        reg[4] = 0x000000000000A536LL;
        reg[12] = 0xFFFFFFFFED10D0B3LL;
        reg[13] = 0x000000001402A4CCLL;
        reg[15] = 0x000000003103E121LL;
        reg[22] = 0x000000000000003FLL;
        reg[25] = 0xFFFFFFFF9DEBB54FLL;
        break;
    case 3:
        reg[1] = 0x0000000000000001LL;
        reg[2] = 0x0000000049A5EE96LL;
        reg[3] = 0x0000000049A5EE96LL;
        reg[4] = 0x000000000000EE96LL;
        reg[12] = 0xFFFFFFFFCE9DFBF7LL;
        reg[13] = 0xFFFFFFFFCE9DFBF7LL;
        reg[15] = 0x0000000018B63D28LL;
        reg[22] = 0x0000000000000078LL;
        reg[25] = 0xFFFFFFFF825B21C9LL;
        break;
    case 5:
        SP_IMEM[0] = 0x3C0DBFC0;
        SP_IMEM[2] = 0x25AD07C0;
        SP_IMEM[3] = 0x31080080;
        SP_IMEM[4] = 0x5500FFFC;
        SP_IMEM[5] = 0x3C0DBFC0;
        SP_IMEM[6] = 0x8DA80024;
        SP_IMEM[7] = 0x3C0BB000;
        reg[2] = 0xFFFFFFFFF58B0FBFLL;
        reg[3] = 0xFFFFFFFFF58B0FBFLL;
        reg[4] = 0x0000000000000FBFLL;
        reg[12] = 0xFFFFFFFF9651F81ELL;
        reg[13] = 0x000000002D42AAC5LL;
        reg[15] = 0x0000000056584D60LL;
        reg[22] = 0x0000000000000091LL;
        reg[25] = 0xFFFFFFFFCDCE565FLL;
        break;
    case 6:
        reg[2] = 0xFFFFFFFFA95930A4LL;
        reg[3] = 0xFFFFFFFFA95930A4LL;
        reg[4] = 0x00000000000030A4LL;
        reg[12] = 0xFFFFFFFFBCB59510LL;
        reg[13] = 0xFFFFFFFFBCB59510LL;
        reg[15] = 0x000000007A3C07F4LL;
        reg[22] = 0x0000000000000085LL;
        reg[25] = 0x00000000465E3F72LL;
        break;
    }

    rounding_mode = MUP_ROUND_NEAREST;
    set_rounding();

    last_addr = 0xa4000040;
    // next_interrupt = 624999; //this is later overwritten with different value so what's the point...
    init_interrupt();
    interpcore = 0;

    if (!dynacore)
    {
        g_core->log_info(L"interpreter");
        init_blocks();
        last_addr = PC->addr;
        core_executing = true;
        g_core->callbacks.core_executing_changed(core_executing);
        g_core->log_info(std::format(L"core_executing: {}", (bool)core_executing));
        while (!stop)
        {
            PC->ops();
            g_vr_beq_ignore_jmp = false;
        }
    }
    else if (dynacore == 2)
    {
        dynacore = 0;
        interpcore = 1;
        pure_interpreter();
    }
    else
    {
        dynacore = 1;
        g_core->log_info(L"dynamic recompiler");
        init_blocks();

        auto code_addr = actual->code + (actual->block[0x40 / 4].local_addr);

        code = (void (*)(void))(code_addr);
        dyna_start(code);
        PC++;
    }
    debug_count += core_Count;
    print_stop_debug();
    for (i = 0; i < 0x100000; i++)
    {
        if (blocks[i] != NULL)
        {
            if (blocks[i]->block)
            {
                free(blocks[i]->block);
                blocks[i]->block = NULL;
            }
            if (blocks[i]->code)
            {
                free_exec(blocks[i]->code);
                blocks[i]->code = NULL;
            }
            if (blocks[i]->jumps_table)
            {
                free(blocks[i]->jumps_table);
                blocks[i]->jumps_table = NULL;
            }
            free(blocks[i]);
            blocks[i] = NULL;
        }
    }
    if (!dynacore && interpcore)
        free(PC);
    core_executing = false;
    g_core->callbacks.core_executing_changed(core_executing);
}

bool open_core_file_stream(const std::filesystem::path& path, FILE** file)
{
    g_core->log_info(std::format(L"[Core] Opening core stream from {}...", path.wstring()));

    if (!exists(path))
    {
        FILE* f = fopen(path.string().c_str(), "w");
        if (!f)
        {
            return false;
        }
        fflush(f);
        fclose(f);
    }
    *file = fopen(path.string().c_str(), "rb+");
    return *file != nullptr;
}


void clear_save_data()
{
    open_core_file_stream(get_eeprom_path(), &g_eeprom_file);
    open_core_file_stream(get_sram_path(), &g_sram_file);
    open_core_file_stream(get_flashram_path(), &g_fram_file);
    open_core_file_stream(get_mempak_path(), &g_mpak_file);

    {
        memset(sram, 0, sizeof(sram));
        fseek(g_sram_file, 0, SEEK_SET);
        fwrite(sram, 1, 0x8000, g_sram_file);
    }
    {
        memset(eeprom, 0, sizeof(eeprom));
        fseek(g_eeprom_file, 0, SEEK_SET);
        fwrite(eeprom, 1, 0x800, g_eeprom_file);
    }
    {
        fseek(g_mpak_file, 0, SEEK_SET);
        for (auto buf : mempack)
        {
            memset(buf, 0, sizeof(mempack) / 4);
            fwrite(buf, 1, 0x800, g_mpak_file);
        }
    }

    fclose(g_eeprom_file);
    fclose(g_sram_file);
    fclose(g_fram_file);
    fclose(g_mpak_file);
}

void audio_thread()
{
    g_core->log_info(L"Sound thread entering...");
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (audio_thread_stop_requested == true)
        {
            break;
        }

        if (g_vr_fast_forward && g_core->cfg->fastforward_silent)
        {
            continue;
        }

        if (core_vcr_is_seeking())
        {
            continue;
        }

        g_core->plugin_funcs.audio_ai_update(0);
    }
    g_core->log_info(L"Sound thread exiting...");
}

void emu_thread()
{
    auto start_time = std::chrono::high_resolution_clock::now();

    g_core->initiate_plugins();

    init_memory();

    g_core->plugin_funcs.video_rom_open();
    g_core->plugin_funcs.input_rom_open();
    g_core->plugin_funcs.audio_rom_open();

    dynacore = g_core->cfg->core_type;

    audio_thread_handle = std::thread(audio_thread);

    g_core->callbacks.emu_launched_changed(true);
    g_core->callbacks.emu_starting_changed(false);
    g_core->callbacks.reset();

    g_core->log_info(std::format(L"[Core] Emu thread entry took {}ms", static_cast<int32_t>((std::chrono::high_resolution_clock::now() - start_time).count() / 1'000'000)));
    core_start();

    st_on_core_stop();

    g_core->plugin_funcs.video_rom_closed();
    g_core->plugin_funcs.audio_rom_closed();
    g_core->plugin_funcs.input_rom_closed();
    g_core->plugin_funcs.rsp_rom_closed();
    g_core->plugin_funcs.video_close_dll();
    g_core->plugin_funcs.audio_close_dll_audio();
    g_core->plugin_funcs.input_close_dll();
    g_core->plugin_funcs.rsp_close_dll();

    emu_paused = true;
    emu_launched = false;

    if (!emu_resetting)
    {
        g_core->callbacks.emu_launched_changed(false);
    }
}


core_result vr_close_rom_impl(bool stop_vcr)
{
    if (!emu_launched)
    {
        return VR_NotRunning;
    }

    core_vr_resume_emu();

    audio_thread_stop_requested = true;
    audio_thread_handle.join();
    audio_thread_stop_requested = false;

    if (stop_vcr)
    {
        core_vcr_stop_all();
    }

    g_core->callbacks.emu_stopping();

    g_core->log_info(L"[Core] Stopping emulation thread...");

    // we signal the core to stop, then wait until thread exits
    terminate_emu();

    emu_thread_handle.join();

    fflush(g_eeprom_file);
    fflush(g_sram_file);
    fflush(g_fram_file);
    fflush(g_mpak_file);
    fclose(g_eeprom_file);
    fclose(g_sram_file);
    fclose(g_fram_file);
    fclose(g_mpak_file);

    return Res_Ok;
}

core_result vr_start_rom_impl(std::filesystem::path path)
{
    auto start_time = std::chrono::high_resolution_clock::now();

    // We can't overwrite core. Emu needs to stop first, but that might fail...
    if (emu_launched)
    {
        auto result = vr_close_rom_impl(true);
        if (result != Res_Ok)
        {
            g_core->log_info(L"[Core] Failed to close rom before starting rom.");
            return result;
        }
    }

    g_core->callbacks.emu_starting_changed(true);

    // If we get a movie instead of a rom, we try to search the available rom lists to find one matching the movie
    if (path.extension() == ".m64")
    {
        core_vcr_movie_header movie_header{};
        const auto result = core_vcr_parse_header(path, &movie_header);
        if (result != Res_Ok)
        {
            g_core->callbacks.emu_starting_changed(false);
            return result;
        }

        const auto matching_rom = g_core->find_available_rom([&](auto header) {
            strtrim((char*)header.nom, sizeof(header.nom));
            return movie_header.rom_crc1 == header.CRC1 && !_strnicmp((const char*)header.nom, movie_header.rom_name, 20);
        });

        if (matching_rom.empty())
        {
            g_core->callbacks.emu_starting_changed(false);
            return VR_NoMatchingRom;
        }

        path = matching_rom;
    }

    rom_path = path;

    if (!g_core->load_plugins())
    {
        g_core->callbacks.emu_starting_changed(false);
        return VR_PluginError;
    }

    if (!rom_load(path.string().c_str()))
    {
        g_core->callbacks.emu_starting_changed(false);
        return VR_RomInvalid;
    }

    // Open all the save file streams
    if (!open_core_file_stream(get_eeprom_path(), &g_eeprom_file) || !open_core_file_stream(get_sram_path(), &g_sram_file) || !open_core_file_stream(get_flashram_path(), &g_fram_file) || !open_core_file_stream(get_mempak_path(), &g_mpak_file))
    {
        g_core->callbacks.emu_starting_changed(false);
        return VR_FileOpenFailed;
    }

    core_vr_on_speed_modifier_changed();

    g_core->log_info(std::format(L"[Core] vr_start_rom entry took {}ms", static_cast<int32_t>((std::chrono::high_resolution_clock::now() - start_time).count() / 1'000'000)));

    emu_paused = false;
    emu_launched = true;
    emu_thread_handle = std::thread(emu_thread);

    // We need to wait until the core is actually done and running before we can continue, because we release the lock
    // If we return too early (before core is ready to also be killed), then another start or close might come in during the core initialization (catastrophe)
    while (!core_executing)
        ;

    return Res_Ok;
}

core_result core_vr_start_rom(std::filesystem::path path)
{
    std::lock_guard lock(g_emu_cs);
    return vr_start_rom_impl(path);
}

core_result core_vr_close_rom(bool stop_vcr)
{
    std::lock_guard lock(g_emu_cs);
    return vr_close_rom_impl(stop_vcr);
}

core_result vr_reset_rom_impl(bool reset_save_data, bool stop_vcr, bool skip_reset_recording_check)
{
    if (!emu_launched)
        return VR_NotRunning;

    // Special case:
    // If we're recording a movie and have reset recording enabled, we don't reset immediately, but let the VCR
    // handle the reset instead. This is to ensure that the movie file is properly closed and saved.
    const auto task = core_vcr_get_task();
    if (g_core->cfg->is_reset_recording_enabled && !skip_reset_recording_check && task == task_recording)
    {
        g_core->log_trace(L"vr_reset_rom_impl Reset during recording, handing off to VCR");
        vcr_reset_requested = true;
        return Res_Ok;
    }

    // why is it so damned difficult to reset the game?
    // right now it's hacked to exit to the GUI then re-load the ROM,
    // but it should be possible to reset the game while it's still running
    // simply by clearing out some memory and maybe notifying the plugins...
    frame_advance_outstanding = 0;
    emu_resetting = true;

    core_result result = core_vr_close_rom(stop_vcr);
    if (result != Res_Ok)
    {
        emu_resetting = false;
        g_core->callbacks.reset_completed();
        return result;
    }

    if (reset_save_data)
    {
        clear_save_data();
    }

    result = core_vr_start_rom(rom_path);
    if (result != Res_Ok)
    {
        emu_resetting = false;
        g_core->callbacks.reset_completed();
        return result;
    }

    emu_resetting = false;
    g_core->callbacks.reset_completed();
    return Res_Ok;
}

// https://github.com/mupen64plus/mupen64plus-core/blob/e170c409fb006aa38fd02031b5eefab6886ec125/src/device/r4300/recomp.c#L995

void* malloc_exec(size_t size)
{
#ifdef WIN32
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
    assert(false);
#endif
}

void* realloc_exec(void* ptr, size_t oldsize, size_t newsize)
{
    void* block = malloc_exec(newsize);
    if (block != NULL)
    {
        size_t copysize;
        copysize = (oldsize < newsize)
        ? oldsize
        : newsize;
        memcpy(block, ptr, copysize);
    }
    free_exec(ptr);
    return block;
}

void free_exec(void* ptr)
{
#ifdef WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    assert(false);
#endif
}

core_result core_vr_reset_rom(bool reset_save_data, bool stop_vcr)
{
    std::lock_guard lock(g_emu_cs);
    return vr_reset_rom_impl(reset_save_data, stop_vcr);
}

void core_vr_toggle_fullscreen_mode()
{
    g_core->plugin_funcs.video_change_window();
    fullscreen ^= true;
}

void core_vr_set_fast_forward(bool value)
{
    g_vr_fast_forward = value;
}

bool core_vr_is_fullscreen()
{
    return fullscreen;
}

bool core_vr_get_gs_button()
{
    return gs_button;
}

void core_vr_set_gs_button(bool value)
{
    gs_button = value;
}
