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
extern unsigned char isGoldeneyeRom;

/**
 * \brief Reads the specified rom and initializes the rom module's globals
 * \param path The rom's path
 * \return Whether the operation succeeded
 */
bool rom_load(std::filesystem::path path);

/**
 * \brief Performs an in-place byte order correction on a rom buffer depending on its header
 * \param rom The rom buffer
 */
void rom_byteswap(uint8_t* rom);
