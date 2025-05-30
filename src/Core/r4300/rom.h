/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <include/core_api.h>

extern uint8_t* rom;
extern size_t rom_size;
extern char rom_md5[33];
extern core_rom_header ROM_HEADER;

/**
 * \brief Reads the specified rom and initializes the rom module's globals
 * \param path The rom's path
 * \return Whether the operation succeeded
 */
bool rom_load(std::filesystem::path path);

void rom_byteswap(uint8_t* rom);

core_rom_header* rom_get_rom_header();
uint32_t rom_get_vis_per_second(uint16_t country_code);
std::wstring rom_country_code_to_country_name(uint16_t country_code);
