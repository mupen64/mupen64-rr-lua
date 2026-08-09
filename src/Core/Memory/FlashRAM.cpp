/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CommonPCH.hpp>
#include "Memory.hpp"
#include <Core.hpp>
#include <R4300/R4300.hpp>

int32_t use_flashram;

typedef enum flashram_mode
{
    NOPES_MODE = 0,
    ERASE_MODE,
    WRITE_MODE,
    READ_MODE,
    STATUS_MODE
} Flashram_mode;

static int32_t mode;
static uint64_t status;
static uint32_t erase_offset, write_pointer;

void save_flashram_infos(char *buf)
{
    memcpy(buf + 0, &use_flashram, 4);
    memcpy(buf + 4, &mode, 4);
    memcpy(buf + 8, &status, 8);
    memcpy(buf + 16, &erase_offset, 4);
    memcpy(buf + 20, &write_pointer, 4);
}

void load_flashram_infos(char *buf)
{
    memcpy(&use_flashram, buf + 0, 4);
    memcpy(&mode, buf + 4, 4);
    memcpy(&status, buf + 8, 8);
    memcpy(&erase_offset, buf + 16, 4);
    memcpy(&write_pointer, buf + 20, 4);
}

bool check_flashram_infos(uint8_t *buf)
{
    uint32_t erase_offset, write_pointer;
    memcpy(&erase_offset, buf + 16, 4);
    memcpy(&write_pointer, buf + 20, 4);

    return erase_offset <= sizeof(flashram) - 128 && write_pointer <= sizeof(rdram) - 128;
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
        case ERASE_MODE: {
            if (erase_offset > sizeof(flashram) - 128) break;

            fseek(g_sram_file, 0, SEEK_SET);
            fread(flashram, 1, 0x20000, g_sram_file);

            for (int32_t i = erase_offset; i < (erase_offset + 128); i++) flashram[i ^ S8] = 0xff;

            fseek(g_sram_file, 0, SEEK_SET);
            fwrite(flashram, 1, 0x20000, g_sram_file);
        }
        break;
        case WRITE_MODE: {
            if (erase_offset > sizeof(flashram) - 128 || write_pointer > sizeof(rdram) - 128) break;

            fseek(g_sram_file, 0, SEEK_SET);
            fread(flashram, 1, 0x20000, g_sram_file);

            for (int32_t i = 0; i < 128; i++)
                flashram[(erase_offset + i) ^ S8] = ((unsigned char *)rdram)[(write_pointer + i) ^ S8];

            fseek(g_sram_file, 0, SEEK_SET);
            fwrite(flashram, 1, 0x20000, g_sram_file);
        }
        break;
        case STATUS_MODE:
            break;
        default:
            critical_stop(std::format("Unknown flashram command with mode {:#06x}", (int32_t)mode));
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
        g_core->log_warn(std::format("unknown flashram command:{:#06x}", (int32_t)command));
        // stop=1;
    }
}

void dma_read_flashram()
{
    int32_t i;

    switch (mode)
    {
    case STATUS_MODE:
        if (pi_register.pi_dram_addr_reg <= sizeof(rdram) - 8)
        {
            rdram[pi_register.pi_dram_addr_reg / 4] = (uint32_t)(status >> 32);
            rdram[pi_register.pi_dram_addr_reg / 4 + 1] = (uint32_t)(status);
        }
        break;
    case READ_MODE: {
        const uint32_t flash_offset = ((pi_register.pi_cart_addr_reg - 0x08000000) & 0xFFFF) * 2;
        uint32_t length = (pi_register.pi_wr_len_reg & 0x0FFFFFF) + 1;
        length = flash_offset < sizeof(flashram) ? std::min(length, (uint32_t)sizeof(flashram) - flash_offset) : 0;
        length = pi_register.pi_dram_addr_reg < sizeof(rdram)
                     ? std::min(length, (uint32_t)sizeof(rdram) - pi_register.pi_dram_addr_reg)
                     : 0;

        fseek(g_fram_file, 0, SEEK_SET);
        fread(flashram, 1, 0x20000, g_fram_file);

        for (uint32_t i = 0; i < length; i++)
            ((unsigned char *)rdram)[(pi_register.pi_dram_addr_reg + i) ^ S8] = flashram[(flash_offset + i) ^ S8];
        break;
    }
    default:
        critical_stop(std::format("Unknown dma_read_flashram {:#06x}", mode));
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
        critical_stop(std::format("Unknown dma_read_flashram {:#06x}", mode));
    }
}
