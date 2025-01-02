/**
 * Mupen64 - rom.h
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

#include <filesystem>
#include <shared/types/CoreTypes.h>
#include <string>

extern uint8_t* rom;
extern size_t rom_size;
extern char rom_md5[33];
extern t_rom_header ROM_HEADER;

/**
 * \brief Reads the specified rom and initializes the rom module's globals
 * \param path The rom's path
 * \return Whether the operation succeeded
 */
bool rom_load(std::filesystem::path path);

/**
 * \param country_code A rom's country code
 * \return The rom's country name
 */
std::wstring country_code_to_country_name(uint16_t country_code);

/**
 * \param country_code A rom's country code
 * \return The maximum amount of VIs per second intended
 */
uint32_t get_vis_per_second(uint16_t country_code);

/**
 * \brief Performs an in-place byte order correction on a rom buffer depending
 * on its header \param rom The rom buffer
 */
void rom_byteswap(uint8_t* rom);
