/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <memory/memory.h>
#include <memory/pif.h>
#include <memory/pif_lut.h>
#include <memory/savestates.h>
#include <cheats.h>
#include <r4300/r4300.h>
#include <r4300/vcr.h>

// Amount of VIs since last input poll
size_t lag_count;

#ifdef DEBUG_PIF
void print_pif()
{
    int32_t i;
    for (i = 0; i < (64 / 8); i++)
        g_core->log_info(L"{:#06x} {:#06x} {:#06x} {:#06x} | {:#06x} {:#06x} {:#06x} {:#06x}",
                            PIF_RAMb[i * 8 + 0], PIF_RAMb[i * 8 + 1], PIF_RAMb[i * 8 + 2], PIF_RAMb[i * 8 + 3],
                            PIF_RAMb[i * 8 + 4], PIF_RAMb[i * 8 + 5], PIF_RAMb[i * 8 + 6], PIF_RAMb[i * 8 + 7]);
    // getchar();
}
#endif

// 16kb eeprom flag
#define EXTENDED_EEPROM (0)

void EepromCommand(uint8_t* Command)
{
    switch (Command[2])
    {
    case 0: // check
        if (Command[1] != 3)
        {
            Command[1] |= 0x40;
            if ((Command[1] & 3) > 0)
                Command[3] = 0;
            if ((Command[1] & 3) > 1)
                Command[4] = EXTENDED_EEPROM == 0 ? 0x80 : 0xc0;
            if ((Command[1] & 3) > 2)
                Command[5] = 0;
        }
        else
        {
            Command[3] = 0;
            Command[4] = EXTENDED_EEPROM == 0 ? 0x80 : 0xc0;
            Command[5] = 0;
        }
        break;
    case 4: // read
        {
            fseek(g_eeprom_file, 0, SEEK_SET);
            fread(eeprom, 1, 0x800, g_eeprom_file);
            memcpy(&Command[4], eeprom + Command[3] * 8, 8);
        }
        break;
    case 5: // write
        {
            fseek(g_eeprom_file, 0, SEEK_SET);
            fread(eeprom, 1, 0x800, g_eeprom_file);
            memcpy(eeprom + Command[3] * 8, &Command[4], 8);

            fseek(g_eeprom_file, 0, SEEK_SET);
            fwrite(eeprom, 1, 0x800, g_eeprom_file);
        }
        break;
    default:
        g_core->log_warn(std::format(L"unknown command in EepromCommand : {:#06x}", Command[2]));
    }
}

void format_mempacks()
{
    unsigned char init[] =
    {
    0x81, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0xff, 0xff, 0xff, 0xff, 0x05, 0x1a, 0x5f, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0xff, 0x66, 0x25, 0x99, 0xcd,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0x05, 0x1a, 0x5f, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0xff, 0x66, 0x25, 0x99, 0xcd,
    0xff, 0xff, 0xff, 0xff, 0x05, 0x1a, 0x5f, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0xff, 0x66, 0x25, 0x99, 0xcd,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0x05, 0x1a, 0x5f, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0xff, 0x66, 0x25, 0x99, 0xcd,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x71, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03};
    int32_t i, j;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 0x8000; j += 2)
        {
            mempack[i][j] = 0;
            mempack[i][j + 1] = 0x03;
        }
        memcpy(mempack[i], init, 272);
    }
}

unsigned char mempack_crc(unsigned char* data)
{
    int32_t i;
    unsigned char CRC = 0;
    for (i = 0; i <= 0x20; i++)
    {
        int32_t mask;
        for (mask = 0x80; mask >= 1; mask >>= 1)
        {
            int32_t xor_tap = (CRC & 0x80) ? 0x85 : 0x00;
            CRC <<= 1;
            if (i != 0x20 && (data[i] & mask))
                CRC |= 1;
            CRC ^= xor_tap;
        }
    }
    return CRC;
}


void internal_ReadController(int32_t Control, uint8_t* Command)
{
    switch (Command[2])
    {
    case 1:
        if (g_core->controls[Control].Present)
        {
            cht_execute();

            lag_count = 0;
            core_buttons input = {0};
            vcr_on_controller_poll(Control, &input);
            *((uint32_t*)(Command + 3)) = input.value;
        }
        break;
    case 2: // read controller pack
    case 3: // write controller pack
        if (g_core->controls[Control].Present)
        {
            if (g_core->controls[Control].Plugin == (int32_t)ce_raw && g_core->plugin_funcs.input_controller_command)
                g_core->plugin_funcs.input_read_controller(Control, Command);
        }
        break;
    }
}

void internal_ControllerCommand(int32_t Control, uint8_t* Command)
{
    switch (Command[2])
    {
    case 0x00: // check
    case 0xFF:
        if ((Command[1] & 0x80))
            break;
        if (g_core->controls[Control].Present)
        {
            Command[3] = 0x05;
            Command[4] = 0x00;
            switch (g_core->controls[Control].Plugin)
            {
            case (int32_t)ce_mempak:
                Command[5] = 1;
                break;
            case (int32_t)ce_raw:
                Command[5] = 1;
                break;
            default:
                Command[5] = 0;
                break;
            }
        }
        else
            Command[1] |= 0x80;
        break;
    case 0x01:
        if (!g_core->controls[Control].Present)
            Command[1] |= 0x80;
        break;
    case 0x02: // read controller pack
        if (g_core->controls[Control].Present)
        {
            switch (g_core->controls[Control].Plugin)
            {
            case (int32_t)ce_mempak:
                {
                    int32_t address = (Command[3] << 8) | Command[4];
                    if (address == 0x8001)
                    {
                        memset(&Command[5], 0, 0x20);
                        Command[0x25] = mempack_crc(&Command[5]);
                    }
                    else
                    {
                        address &= 0xFFE0;
                        if (address <= 0x7FE0)
                        {
                            fseek(g_mpak_file, 0, SEEK_SET);
                            fread(mempack[0], 1, 0x8000, g_mpak_file);
                            fread(mempack[1], 1, 0x8000, g_mpak_file);
                            fread(mempack[2], 1, 0x8000, g_mpak_file);
                            fread(mempack[3], 1, 0x8000, g_mpak_file);

                            memcpy(&Command[5], &mempack[Control][address], 0x20);
                        }
                        else
                        {
                            memset(&Command[5], 0, 0x20);
                        }
                        Command[0x25] = mempack_crc(&Command[5]);
                    }
                }
                break;
            case (int32_t)ce_raw:
                if (g_core->plugin_funcs.input_controller_command)
                    g_core->plugin_funcs.input_controller_command(Control, Command);
                break;
            default:
                memset(&Command[5], 0, 0x20);
                Command[0x25] = 0;
            }
        }
        else
            Command[1] |= 0x80;
        break;
    case 0x03: // write controller pack
        if (g_core->controls[Control].Present)
        {
            switch (g_core->controls[Control].Plugin)
            {
            case (int32_t)ce_mempak:
                {
                    int32_t address = (Command[3] << 8) | Command[4];
                    if (address == 0x8001)
                        Command[0x25] = mempack_crc(&Command[5]);
                    else
                    {
                        address &= 0xFFE0;
                        if (address <= 0x7FE0)
                        {
                            fseek(g_mpak_file, 0, SEEK_SET);
                            fread(mempack[0], 1, 0x8000, g_mpak_file);
                            fread(mempack[1], 1, 0x8000, g_mpak_file);
                            fread(mempack[2], 1, 0x8000, g_mpak_file);
                            fread(mempack[3], 1, 0x8000, g_mpak_file);

                            memcpy(&mempack[Control][address], &Command[5], 0x20);

                            fseek(g_mpak_file, 0, SEEK_SET);
                            fwrite(mempack[0], 1, 0x8000, g_mpak_file);
                            fwrite(mempack[1], 1, 0x8000, g_mpak_file);
                            fwrite(mempack[2], 1, 0x8000, g_mpak_file);
                            fwrite(mempack[3], 1, 0x8000, g_mpak_file);
                        }
                        Command[0x25] = mempack_crc(&Command[5]);
                    }
                }
                break;
            case (int32_t)ce_raw:
                if (g_core->plugin_funcs.input_controller_command)
                    g_core->plugin_funcs.input_controller_command(Control, Command);
                break;
            default:
                Command[0x25] = mempack_crc(&Command[5]);
            }
        }
        else
            Command[1] |= 0x80;
        break;
    }
}

void update_pif_write()
{
    int32_t i = 0, channel = 0;
    /*#ifdef DEBUG_PIF
        if (input_delay) {
            g_core->log_info(L"------------- write -------------");
        }
        else {
            g_core->log_info(L"---------- before write ---------");
        }
        print_pif();
        g_core->log_info(L"---------------------------------");
    #endif*/
    if (PIF_RAMb[0x3F] > 1)
    {
        switch (PIF_RAMb[0x3F])
        {
        case 0x02:
            for (i = 0; i < sizeof(g_pif_lut) / 32; i++)
            {
                if (!memcmp(PIF_RAMb + 64 - 2 * 8, g_pif_lut[i][0], 16))
                {
                    memcpy(PIF_RAMb + 64 - 2 * 8, g_pif_lut[i][1], 16);
                    return;
                }
            }
            g_core->log_info(L"unknown pif2 code:");
            for (i = (64 - 2 * 8) / 8; i < (64 / 8); i++)
            {
                g_core->log_info(std::format(L"{:#06x} {:#06x} {:#06x} {:#06x} | {:#06x} {:#06x} {:#06x} {:#06x}", PIF_RAMb[i * 8 + 0], PIF_RAMb[i * 8 + 1], PIF_RAMb[i * 8 + 2], PIF_RAMb[i * 8 + 3], PIF_RAMb[i * 8 + 4], PIF_RAMb[i * 8 + 5], PIF_RAMb[i * 8 + 6], PIF_RAMb[i * 8 + 7]));
            }
            break;
        case 0x08:
            PIF_RAMb[0x3F] = 0;
            break;
        default:
            g_core->log_info(std::format(L"error in update_pif_write : {:#06x}", PIF_RAMb[0x3F]));
        }
        return;
    }
    while (i < 0x40)
    {
        switch (PIF_RAMb[i])
        {
        case 0x00:
            channel++;
            if (channel > 6)
                i = 0x40;
            break;
        case 0xFF:
            break;
        default:
            if (!(PIF_RAMb[i] & 0xC0))
            {
                if (channel < 4)
                {
                    if (g_core->controls[channel].Present &&
                        g_core->controls[channel].RawData)
                        g_core->plugin_funcs.input_controller_command(channel, &PIF_RAMb[i]);
                    else
                        internal_ControllerCommand(channel, &PIF_RAMb[i]);
                }
                else if (channel == 4)
                    EepromCommand(&PIF_RAMb[i]);
                else
                    g_core->log_info(L"channel >= 4 in update_pif_write");
                i += PIF_RAMb[i] + (PIF_RAMb[(i + 1)] & 0x3F) + 1;
                channel++;
            }
            else
                i = 0x40;
        }
        i++;
    }
    // PIF_RAMb[0x3F] = 0;
    g_core->plugin_funcs.input_controller_command(-1, NULL);
    /*#ifdef DEBUG_PIF
        if (!one_frame_delay) {
            g_core->log_info(L"---------- after write ----------");
        }
        print_pif();
        if (!one_frame_delay) {
            g_core->log_info(L"---------------------------------");
        }
    #endif*/
}


void update_pif_read()
{
    // g_core->log_info(L"pif entry");
    int32_t i = 0, channel = 0;
    bool once = emu_paused || (frame_advance_outstanding > 0) || g_wait_counter; // used to pause only once during controller routine
    bool stAllowed = true; // used to disallow .st being loaded after any controller has already been read
#ifdef DEBUG_PIF
    g_core->log_info(L"---------- before read ----------");
    print_pif();
    g_core->log_info(L"---------------------------------");
#endif
    while (i < 0x40)
    {
        switch (PIF_RAMb[i])
        {
        case 0x00:
            channel++;
            if (channel > 6)
                i = 0x40;
            break;
        case 0xFE:
            i = 0x40;
            break;
        case 0xFF:
            break;
        case 0xB4:
        case 0x56:
        case 0xB8:
            break;
        default:
            // 01 04 01 is read controller 4 bytes
            if (!(PIF_RAMb[i] & 0xC0)) // mask error bits (isn't this wrong? error bits are on i+1???)
            {
                if (channel < 4)
                {
                    static int32_t controllerRead = 999;

                    // frame advance - pause before every 'frame of input',
                    // which is manually resumed to enter 1 input and emulate until being
                    // paused here again before the next input
                    if (once && channel <= controllerRead && (&PIF_RAMb[i])[2] == 1)
                    {
                        once = false;

                        if (g_wait_counter == 0)
                        {
                            if (frame_advance_outstanding == 1)
                            {
                                --frame_advance_outstanding;
                                core_vr_pause_emu();
                            } else if (frame_advance_outstanding > 1)
                            {
                                --frame_advance_outstanding;
                            }
                        }

                        while (g_wait_counter)
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                            if (stAllowed)
                            {
                                st_do_work();
                            }
                        }

                        while (emu_paused)
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                            
                            g_core->callbacks.interval();

                            if (stAllowed)
                            {
                                st_do_work();
                            }
                        }
                    }
                    if (stAllowed)
                    {
                        st_do_work();
                    }
                    if (g_st_old)
                    {
                        // if old savestate, don't fetch controller (matches old behaviour), makes delay fix not work for that st but syncs all m64s
                        g_core->log_info(L"old st detected");
                        g_st_old = false;
                        return;
                    }
                    stAllowed = false;
                    controllerRead = channel;

                    // we handle raw data-mode controllers here:
                    // this is incompatible with VCR!
                    if (g_core->controls[channel].Present &&
                        g_core->controls[channel].RawData && core_vcr_get_task() == task_idle)
                    {
                        g_core->plugin_funcs.input_read_controller(channel, &PIF_RAMb[i]);
                        auto ptr = (core_buttons*)&PIF_RAMb[i + 3];
                        g_core->callbacks.input(ptr, channel);
                    }
                    else
                        internal_ReadController(channel, &PIF_RAMb[i]);
                }
                i += PIF_RAMb[i] + (PIF_RAMb[(i + 1)] & 0x3F) + 1;
                channel++;
            }
            else
                i = 0x40;
        }
        i++;
    }
    g_core->plugin_funcs.input_read_controller(-1, NULL);

#ifdef DEBUG_PIF
    g_core->log_info(L"---------- after read -----------");
    print_pif();
    g_core->log_info(L"---------------------------------");
#endif
    // g_core->log_info(L"pif exit");
}
