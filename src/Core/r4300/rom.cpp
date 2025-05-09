/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <IOHelpers.h>
#include <md5.h>
#include <memory/memory.h>
#include <r4300/r4300.h>
#include <r4300/rom.h>

std::unordered_map<std::filesystem::path, std::pair<uint8_t*, size_t>> rom_cache;

uint8_t* rom;
size_t rom_size;
char rom_md5[33];

core_rom_header ROM_HEADER;

void print_rom_info()
{
    g_core->log_info(L"--- Rom Info ---");
    g_core->log_info(std::format(L"{:#06x} {:#06x} {:#06x} {:#06x}", ROM_HEADER.init_PI_BSB_DOM1_LAT_REG, ROM_HEADER.init_PI_BSB_DOM1_PGS_REG, ROM_HEADER.init_PI_BSB_DOM1_PWD_REG, ROM_HEADER.init_PI_BSB_DOM1_PGS_REG2));
    g_core->log_info(std::format(L"Clock rate: {:#06x}", sl((uint32_t)ROM_HEADER.ClockRate)));
    g_core->log_info(std::format(L"Version: {:#06x}", sl((uint32_t)ROM_HEADER.Release)));
    g_core->log_info(std::format(L"CRC: {:#06x} {:#06x}", sl((uint32_t)ROM_HEADER.CRC1), sl((uint32_t)ROM_HEADER.CRC2)));
    g_core->log_info(std::format(L"Name: {}", string_to_wstring((char*)ROM_HEADER.nom)));
    if (sl(ROM_HEADER.Manufacturer_ID) == 'N')
        g_core->log_info(L"Manufacturer: Nintendo");
    else
        g_core->log_info(std::format(L"Manufacturer: {:#06x}", ROM_HEADER.Manufacturer_ID));
    g_core->log_info(std::format(L"Cartridge ID: {:#06x}", ROM_HEADER.Cartridge_ID));
    g_core->log_info(std::format(L"Size: {}", rom_size));
    g_core->log_info(std::format(L"PC: {:#06x}\n", sl((uint32_t)ROM_HEADER.PC)));
    g_core->log_info(std::format(L"Country: {}", core_vr_country_code_to_country_name(ROM_HEADER.Country_code)));
    g_core->log_info(L"----------------");
}

std::wstring core_vr_country_code_to_country_name(uint16_t country_code)
{
    switch (country_code & 0xFF)
    {
    case 0:
        return L"Demo";
    case '7':
        return L"Beta";
    case 0x41:
        return L"USA/Japan";
    case 0x44:
        return L"Germany";
    case 0x45:
        return L"USA";
    case 0x46:
        return L"France";
    case 'I':
        return L"Italy";
    case 0x4A:
        return L"Japan";
    case 'S':
        return L"Spain";
    case 0x55:
    case 0x59:
        return L"Australia";
    case 0x50:
    case 0x58:
    case 0x20:
    case 0x21:
    case 0x38:
    case 0x70:
        return L"Europe";
    default:
        return L"Unknown (" + std::to_wstring(country_code & 0xFF) + L")";
    }
}

core_rom_header* core_vr_get_rom_header()
{
    return &ROM_HEADER;
}

uint32_t core_vr_get_vis_per_second(uint16_t country_code)
{
    switch (country_code & 0xFF)
    {
    case 0x44:
    case 0x46:
    case 0x49:
    case 0x50:
    case 0x53:
    case 0x55:
    case 0x58:
    case 0x59:
        return 50;
    case 0x37:
    case 0x41:
    case 0x45:
    case 0x4a:
        return 60;
    default:
        return 60;
    }
}

void core_vr_byteswap(uint8_t* rom)
{
    uint8_t tmp = 0;

    if (rom[0] == 0x37)
    {
        for (size_t i = 0; i < (0x40 / 2); i++)
        {
            tmp = rom[i * 2];
            rom[i * 2] = rom[i * 2 + 1];
            rom[i * 2 + 1] = tmp;
        }
    }
    if (rom[0] == 0x40)
    {
        for (size_t i = 0; i < (0x40 / 4); i++)
        {
            tmp = rom[i * 4];
            rom[i * 4] = rom[i * 4 + 3];
            rom[i * 4 + 3] = tmp;
            tmp = rom[i * 4 + 1];
            rom[i * 4 + 1] = rom[i * 4 + 2];
            rom[i * 4 + 2] = tmp;
        }
    }
}

bool rom_load(std::filesystem::path path)
{
    if (rom)
    {
        free(rom);
        g_core->rom = rom = nullptr;
    }

    if (rom_cache.contains(path))
    {
        g_core->log_info(L"[Core] Loading cached ROM...");
        g_core->rom = rom = (unsigned char*)malloc(rom_cache[path].second);
        memcpy(rom, rom_cache[path].first, rom_cache[path].second);
        return true;
    }

    auto rom_buf = read_file_buffer(path);
    auto decompressed_rom = auto_decompress(rom_buf);

    if (decompressed_rom.empty())
    {
        return false;
    }

    rom_size = decompressed_rom.size();
    uint32_t taille = rom_size;
    if (g_core->cfg->use_summercart && taille < 0x4000000)
        taille = 0x4000000;

    g_core->rom = rom = (unsigned char*)malloc(taille);
    memcpy(rom, decompressed_rom.data(), rom_size);

    uint8_t tmp;
    if (rom[0] == 0x37)
    {
        for (size_t i = 0; i < (rom_size / 2); i++)
        {
            tmp = rom[i * 2];
            rom[i * 2] = rom[i * 2 + 1];
            rom[i * 2 + 1] = (unsigned char)tmp;
        }
    }
    if (rom[0] == 0x40)
    {
        for (size_t i = 0; i < (rom_size / 4); i++)
        {
            tmp = rom[i * 4];
            rom[i * 4] = rom[i * 4 + 3];
            rom[i * 4 + 3] = (unsigned char)tmp;
            tmp = rom[i * 4 + 1];
            rom[i * 4 + 1] = rom[i * 4 + 2];
            rom[i * 4 + 2] = (unsigned char)tmp;
        }
    }
    else if ((rom[0] != 0x80) || (rom[1] != 0x37) || (rom[2] != 0x12) || (rom[3] != 0x40)

    )
    {
        g_core->log_info(L"wrong file format !");
        return false;
    }

    g_core->log_info(L"rom loaded succesfully");

    memcpy(&ROM_HEADER, rom, sizeof(core_rom_header));
    ROM_HEADER.unknown = 0;
    // Clean up ROMs that accidentally set the unused bytes (ensuring previous fields are null terminated)
    ROM_HEADER.Unknown[0] = 0;
    ROM_HEADER.Unknown[1] = 0;

    // trim header
    strtrim((char*)ROM_HEADER.nom, sizeof(ROM_HEADER.nom));

    {
        md5_state_t state;
        md5_byte_t digest[16];
        md5_init(&state);
        md5_append(&state, rom, rom_size);
        md5_finish(&state, digest);

        char arg[256] = {0};
        for (size_t i = 0; i < 16; i++)
            sprintf_s(arg + i * 2, sizeof(arg + i * 2), "%02X", digest[i]);
        strcpy_s(rom_md5, sizeof(rom_md5), arg);
    }

    auto roml = (uint32_t*)rom;
    for (size_t i = 0; i < (rom_size / 4); i++)
        roml[i] = sl(roml[i]);

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
        g_sys_type = sys_pal;
        break;
    case 0x37:
    case 0x41:
    case 0x45:
    case 0x4a:
        g_sys_type = sys_ntsc;
        break;
    default:
        g_core->log_warn(std::format(L"Unknown ccode: {:#06x}. Assuming PAL.", ROM_HEADER.Country_code));
        g_sys_type = sys_pal;
        break;
    }

    if (rom_cache.size() < g_core->cfg->rom_cache_size)
    {
        g_core->log_info(std::format(L"[Core] Putting ROM in cache... ({}/{} full)\n", rom_cache.size(), g_core->cfg->rom_cache_size));
        auto data = (uint8_t*)malloc(taille);
        memcpy(data, rom, taille);
        rom_cache[path] = std::make_pair(data, taille);
    }

    return true;
}
