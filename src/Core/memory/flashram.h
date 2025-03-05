/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

extern int32_t use_flashram;

void init_flashram();
void flashram_command(uint32_t command);
uint32_t flashram_status();
void dma_read_flashram();
void dma_write_flashram();

void save_flashram_infos(char* buf);
void load_flashram_infos(char* buf);
bool check_flashram_infos(uint8_t* buf);