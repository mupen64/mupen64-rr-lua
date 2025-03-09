/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <include/core_types.h>

int32_t init_memory();
constexpr uint32_t ADDR_MASK = 0x7FFFFF;
#define read_word_in_memory() readmem[address>>16]()
#define read_byte_in_memory() readmemb[address>>16]()
#define read_hword_in_memory() readmemh[address>>16]()
#define read_dword_in_memory() readmemd[address>>16]()
#define write_word_in_memory() writemem[address>>16]()
#define write_byte_in_memory() writememb[address >>16]()
#define write_hword_in_memory() writememh[address >>16]()
#define write_dword_in_memory() writememd[address >>16]()
extern uint32_t SP_DMEM[0x1000 / 4 * 2];
extern unsigned char* SP_DMEMb;
extern uint32_t* SP_IMEM;
extern uint32_t PIF_RAM[0x40 / 4];
extern unsigned char* PIF_RAMb;
extern uint32_t rdram[0x800000 / 4];
extern uint8_t* rdramb;
extern uint8_t sram[0x8000];
extern uint8_t flashram[0x20000];
extern uint8_t eeprom[0x800];
extern uint8_t mempack[4][0x8000];
extern uint32_t address, word;
extern unsigned char g_byte;
extern uint16_t hword;
extern uint64_t dword, *rdword;

extern void (*readmem[0xFFFF])();
extern void (*readmemb[0xFFFF])();
extern void (*readmemh[0xFFFF])();
extern void (*readmemd[0xFFFF])();
extern void (*writemem[0xFFFF])();
extern void (*writememb[0xFFFF])();
extern void (*writememh[0xFFFF])();
extern void (*writememd[0xFFFF])();

extern core_rdram_reg rdram_register;
extern core_pi_reg pi_register;
extern core_mips_reg MI_register;
extern core_sp_reg sp_register;
extern core_si_reg si_register;
extern core_vi_reg vi_register;
extern core_rsp_reg rsp_register;
extern core_ri_reg ri_register;
extern core_ai_reg ai_register;
extern core_dpc_reg dpc_register;
extern core_dps_reg dps_register;

#ifndef _BIG_ENDIAN
#define sl(mot) \
( \
((mot & 0x000000FF) << 24) | \
((mot & 0x0000FF00) <<  8) | \
((mot & 0x00FF0000) >>  8) | \
((mot & 0xFF000000) >> 24) \
)

#define S8 3
#define S16 2
#define Sh16 1

#else

#define sl(mot) mot
#define S8 0
#define S16 0
#define Sh16 0

#endif

void read_nothing();
void read_nothingh();
void read_nothingb();
void read_nothingd();
void read_nomem();
void read_nomemb();
void read_nomemh();
void read_nomemd();
void read_rdram();
void read_rdramb();
void read_rdramh();
void read_rdramd();
void read_rdramFB();
void read_rdramFBb();
void read_rdramFBh();
void read_rdramFBd();
void read_rdramreg();
void read_rdramregb();
void read_rdramregh();
void read_rdramregd();
void read_rsp_mem();
void read_rsp_memb();
void read_rsp_memh();
void read_rsp_memd();
void read_rsp_reg();
void read_rsp_regb();
void read_rsp_regh();
void read_rsp_regd();
void read_rsp();
void read_rspb();
void read_rsph();
void read_rspd();
void read_dp();
void read_dpb();
void read_dph();
void read_dpd();
void read_dps();
void read_dpsb();
void read_dpsh();
void read_dpsd();
void read_mi();
void read_mib();
void read_mih();
void read_mid();
void read_vi();
void read_vib();
void read_vih();
void read_vid();
void read_ai();
void read_aib();
void read_aih();
void read_aid();
void read_pi();
void read_pib();
void read_pih();
void read_pid();
void read_ri();
void read_rib();
void read_rih();
void read_rid();
void read_si();
void read_sib();
void read_sih();
void read_sid();
void read_flashram_status();
void read_flashram_statusb();
void read_flashram_statush();
void read_flashram_statusd();
void read_rom();
void read_romb();
void read_romh();
void read_romd();
void read_pif();
void read_pifb();
void read_pifh();
void read_pifd();
void read_sc_reg();
void read_sc_regb();
void read_sc_regh();
void read_sc_regd();

void write_nothing();
void write_nothingb();
void write_nothingh();
void write_nothingd();
void write_nomem();
void write_nomemb();
void write_nomemd();
void write_nomemh();
void write_rdram();
void write_rdramb();
void write_rdramh();
void write_rdramd();
void write_rdramFB();
void write_rdramFBb();
void write_rdramFBh();
void write_rdramFBd();
void write_rdramreg();
void write_rdramregb();
void write_rdramregh();
void write_rdramregd();
void write_rsp_mem();
void write_rsp_memb();
void write_rsp_memh();
void write_rsp_memd();
void write_rsp_reg();
void write_rsp_regb();
void write_rsp_regh();
void write_rsp_regd();
void write_rsp();
void write_rspb();
void write_rsph();
void write_rspd();
void write_dp();
void write_dpb();
void write_dph();
void write_dpd();
void write_dps();
void write_dpsb();
void write_dpsh();
void write_dpsd();
void write_mi();
void write_mib();
void write_mih();
void write_mid();
void write_vi();
void write_vib();
void write_vih();
void write_vid();
void write_ai();
void write_aib();
void write_aih();
void write_aid();
void write_pi();
void write_pib();
void write_pih();
void write_pid();
void write_ri();
void write_rib();
void write_rih();
void write_rid();
void write_si();
void write_sib();
void write_sih();
void write_sid();
void write_flashram_dummy();
void write_flashram_dummyb();
void write_flashram_dummyh();
void write_flashram_dummyd();
void write_flashram_command();
void write_flashram_commandb();
void write_flashram_commandh();
void write_flashram_commandd();
void write_rom();
void write_pif();
void write_pifb();
void write_pifh();
void write_pifd();
void write_sc_reg();
void write_sc_regb();
void write_sc_regh();
void write_sc_regd();

void update_SP();
void update_DPC();

/**
 * \brief Checks whether the provided register contents are valid.
 */
bool check_register_validity(core_si_reg* si_reg);