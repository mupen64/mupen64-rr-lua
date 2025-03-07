/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

struct summercart
{
    char buffer[8192];
    uint32_t status;
    uint32_t data0;
    uint32_t data1;
    uint32_t sd_sector;
    char cfg_rom_write;
    char sd_byteswap;
    char unlock;
    char lock_seq;
    char pad[492];
};

extern struct summercart summercart;

void save_summercart(const std::filesystem::path& path);
void load_summercart(const std::filesystem::path& path);
void init_summercart();
uint32_t read_summercart(uint32_t address);
void write_summercart(uint32_t address, uint32_t value);
