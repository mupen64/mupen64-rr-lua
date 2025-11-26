/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "hle_internal.h"

void hle_init(struct hle_t* hle,
              unsigned char* dram,
              unsigned char* dmem,
              unsigned char* imem,
              unsigned int* mi_intr,
              unsigned int* sp_mem_addr,
              unsigned int* sp_dram_addr,
              unsigned int* sp_rd_length,
              unsigned int* sp_wr_length,
              unsigned int* sp_status,
              unsigned int* sp_dma_full,
              unsigned int* sp_dma_busy,
              unsigned int* sp_pc,
              unsigned int* sp_semaphore,
              unsigned int* dpc_start,
              unsigned int* dpc_end,
              unsigned int* dpc_current,
              unsigned int* dpc_status,
              unsigned int* dpc_clock,
              unsigned int* dpc_bufbusy,
              unsigned int* dpc_pipebusy,
              unsigned int* dpc_tmem,
              void* user_defined);

void hle_execute(struct hle_t* hle);
