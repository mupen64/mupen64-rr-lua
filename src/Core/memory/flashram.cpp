/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "memory.h"
#include <Core.h>
#include <r4300/r4300.h>

int32_t use_flashram;

typedef enum flashram_mode {
    NOPES_MODE = 0,
    ERASE_MODE,
    WRITE_MODE,
    READ_MODE,
    STATUS_MODE
} Flashram_mode;

static int32_t mode;
static uint64_t status;
static uint32_t erase_offset, write_pointer;

void save_flashram_infos(char* buf)
{
    memcpy(buf + 0, &use_flashram, 4);
    memcpy(buf + 4, &mode, 4);
    memcpy(buf + 8, &status, 8);
    memcpy(buf + 16, &erase_offset, 4);
    memcpy(buf + 20, &write_pointer, 4);
}

void load_flashram_infos(char* buf)
{
    memcpy(&use_flashram, buf + 0, 4);
    memcpy(&mode, buf + 4, 4);
    memcpy(&status, buf + 8, 8);
    memcpy(&erase_offset, buf + 16, 4);
    memcpy(&write_pointer, buf + 20, 4);
}

bool check_flashram_infos(uint8_t* buf)
{
    uint32_t erase_offset, write_pointer;
    memcpy(&erase_offset, buf + 16, 4);
    memcpy(&write_pointer, buf + 20, 4);

    for (int32_t i = erase_offset; i < erase_offset + 128; i++)
    {
        const auto mapped_index = i ^ S8;
        if (mapped_index < 0 || mapped_index >= sizeof(flashram))
            return false;
    }

    for (int32_t i = write_pointer; i < write_pointer + 128; i++)
    {
        const auto mapped_index = i ^ S8;
        if (mapped_index < 0 || mapped_index >= sizeof(rdram))
            return false;
    }

    return true;
}

void init_flashram()
{
    mode = NOPES_MODE;
    status = 0;
}

uint32_t flashram_status()
{
    return (status >> 32);
}

void flashram_command(uint32_t command)
{
    switch (command & 0xff000000)
    {
    case 0x4b000000:
        erase_offset = (command & 0xffff) * 128;
        break;
    case 0x78000000:
        mode = ERASE_MODE;
        status = 0x1111800800c20000LL;
        break;
    case 0xa5000000:
        erase_offset = (command & 0xffff) * 128;
        status = 0x1111800400c20000LL;
        break;
    case 0xb4000000:
        mode = WRITE_MODE;
        break;
    case 0xd2000000: // execute
        switch (mode)
        {
        case NOPES_MODE:
            break;
        case ERASE_MODE:
            {
                fseek(g_sram_file, 0, SEEK_SET);
                fread(flashram, 1, 0x20000, g_sram_file);

                for (int32_t i = erase_offset; i < (erase_offset + 128); i++)
                    flashram[i ^ S8] = 0xff;

                fseek(g_sram_file, 0, SEEK_SET);
                fwrite(flashram, 1, 0x20000, g_sram_file);
            }
            break;
        case WRITE_MODE:
            {
                fseek(g_sram_file, 0, SEEK_SET);
                fread(flashram, 1, 0x20000, g_sram_file);

                for (int32_t i = 0; i < 128; i++)
                    flashram[(erase_offset + i) ^ S8] =
                    ((unsigned char*)rdram)[(write_pointer + i) ^ S8];

                fseek(g_sram_file, 0, SEEK_SET);
                fwrite(flashram, 1, 0x20000, g_sram_file);
            }
            break;
        case STATUS_MODE:
            break;
        default:
            g_core->log_warn(std::format(L"unknown flashram command with mode:{:#06x}", (int32_t)mode));
            stop = 1;
        }
        mode = NOPES_MODE;
        break;
    case 0xe1000000:
        mode = STATUS_MODE;
        status = 0x1111800100c20000LL;
        break;
    case 0xf0000000:
        mode = READ_MODE;
        status = 0x11118004f0000000LL;
        break;
    default:
        g_core->log_warn(std::format(L"unknown flashram command:{:#06x}", (int32_t)command));
        // stop=1;
    }
}

void dma_read_flashram()
{
    int32_t i;

    switch (mode)
    {
    case STATUS_MODE:
        rdram[pi_register.pi_dram_addr_reg / 4] = (uint32_t)(status >> 32);
        rdram[pi_register.pi_dram_addr_reg / 4 + 1] = (uint32_t)(status);
        break;
    case READ_MODE:
        {
            fseek(g_fram_file, 0, SEEK_SET);
            fread(flashram, 1, 0x20000, g_fram_file);

            for (i = 0; i < (pi_register.pi_wr_len_reg & 0x0FFFFFF) + 1; i++)
                ((unsigned char*)rdram)[(pi_register.pi_dram_addr_reg + i) ^ S8] =
                flashram[(((pi_register.pi_cart_addr_reg - 0x08000000) & 0xFFFF) * 2 + i) ^ S8];
            break;
        }
    default:
        g_core->log_warn(std::format(L"unknown dma_read_flashram:{:#06x}", mode));
        stop = 1;
    }
}

void dma_write_flashram()
{
    switch (mode)
    {
    case WRITE_MODE:
        write_pointer = pi_register.pi_dram_addr_reg;
        break;
    default:
        g_core->log_warn(std::format(L"unknown dma_read_flashram:{:#06x}", mode));
        stop = 1;
    }
}
