/**
 * Mupen64 - compare_core.c
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

#include <core/r4300/Plugin.hpp>
#include <shared/services/LoggingService.h>
#include <sys/stat.h>
#include "../memory/memory.h"
#include "../r4300/recomph.h"
#include "r4300.h"

static FILE* f;
static int32_t pipe_opened = 0;
static int64_t comp_reg[32];
extern uint32_t interp_addr;
static uint32_t old_op;
int32_t compare_core_mode = 0;

void display_error(const char* txt)
{
    int32_t i;
    uint32_t* comp_reg2 = (uint32_t*)comp_reg;
    g_core_logger->error("err: {}", txt);
    if (interpcore)
    {
        g_core_logger->info("addr:{:#06x}", (int32_t)interp_addr);
        if (!strcmp(txt, "PC"))
            g_core_logger->info("{:#06x} - {:#06x}", (int32_t)interp_addr, *(int32_t*)&comp_reg[0]);
    }
    else
    {
        g_core_logger->info("addr:{:#06x}", (int32_t)PC->addr);
        if (!strcmp(txt, "PC"))
            g_core_logger->info("{:#06x} - {:#06x}", (int32_t)PC->addr, *(int32_t*)&comp_reg[0]);
    }
    g_core_logger->info("{:#06x}, {:#06x}", (uint32_t)reg_cop0[9], (uint32_t)comp_reg2[9]);
    g_core_logger->info("erreur @:{:#06x}", (int32_t)old_op);
    g_core_logger->info("erreur @:{:#06x}", (int32_t)vr_op);

    if (!strcmp(txt, "gpr"))
    {
        for (i = 0; i < 32; i++)
        {
            if (reg[i] != comp_reg[i])
                g_core_logger->info("reg[{}]={:#08x} != reg[{}]={:#08x}", i, reg[i], i, comp_reg[i]);
        }
    }
    if (!strcmp(txt, "cop0"))
    {
        for (i = 0; i < 32; i++)
        {
            if (reg_cop0[i] != comp_reg2[i])
                g_core_logger->info("reg_cop0[{}]={:#06x} != reg_cop0[{}]={:#06x}", i, (uint32_t)reg_cop0[i], i, (uint32_t)comp_reg2[i]);
        }
    }
    /*for (i=0; i<32; i++)
      {
     if (reg_cop0[i] != comp_reg[i])
       g_core_logger->info("reg_cop0[{}]=%llx != reg[{}]=%llx",
          i, reg_cop0[i], i, comp_reg[i]);
      }*/

    terminate_emu();
}

void check_input_sync(unsigned char* value)
{
    if (compare_core_mode == 0)
        return;

    if (compare_core_mode == 3 ? dynacore || interpcore : compare_core_mode == 1)
    {
        fread(value, 4, 1, f);
    }
    else
    {
        fwrite(value, 4, 1, f);
    }
}

void compare_core()
{
    // static int32_t wait=1;

    if (compare_core_mode == 0)
    {
        if (pipe_opened)
        {
            pipe_opened = 0;
            if (f)
                fclose(f);
            f = NULL;
        }
        return;
    }

    if (compare_core_mode == 3 ? dynacore || interpcore : compare_core_mode == 1)
    {
        if (pipe_opened != 1)
        {
#ifdef _MSC_VER
            // TODO: “K“–‚É‘‚¢‚½B‚ ‚ÆACloseHandle()
            // HANDLE pipe = CreateNamedPipe(
            //		"compare_pipe",
            //		PIPE_ACCESS_DUPLEX,
            //		PIPE_WAIT
            //		| PIPE_READMODE_BYTE
            //		| PIPE_TYPE_BYTE,
            //		PIPE_UNLIMITED_INSTANCES,
            //		1024,
            //		1024,
            //		120 * 1000,
            //		NULL);
            //::ConnectNamedPipe(pipe, NULL);
#else
            mkfifo("compare_pipe", 0600);
#endif
            if (f)
                fclose(f);
            f = fopen("compare_pipe", "rb");
            pipe_opened = 1;
        }
        /*if(wait == 1 && reg_cop0[9] > 0x35000000) wait=0;
        if(wait) return;*/

        fread(comp_reg, 1, sizeof(int32_t), f);
        if (feof(f))
        {
            pipe_opened = 0;
            fclose(f);
            f = NULL;
            compare_core_mode = 0;
            return;
        }
        if (interpcore)
        {
            if (memcmp(&interp_addr, comp_reg, 4))
                display_error("PC");
        }
        else
        {
            if (memcmp(&PC->addr, comp_reg, 4))
                display_error("PC");
        }
        /*
        fread (comp_reg, 32, sizeof(int64_t), f);
        if (memcmp(reg, comp_reg, 32*sizeof(int64_t)))
          display_error("gpr");
        fread (comp_reg, 32, sizeof(int32_t), f);
        if (memcmp(reg_cop0, comp_reg, 32*sizeof(int32_t)))
          display_error("cop0");
        fread (comp_reg, 32, sizeof(int64_t), f);
        if (memcmp(reg_cop1_fgr_64, comp_reg, 32*sizeof(int64_t)))
          display_error("cop1");
          */
        /*fread(comp_reg, 1, sizeof(int32_t), f);
        if (memcmp(&rdram[0x31280/4], comp_reg, sizeof(int32_t)))
          display_error("mem");*/
        /*fread (comp_reg, 4, 1, f);
        if (memcmp(&FCR31, comp_reg, 4))
          display_error();*/
        old_op = vr_op;
    }
    else
    {
        // if (reg_cop0[9] > 0x6800000) g_core_logger->info("PC={:#06x}",
        // (int32_t)PC->addr);
        if (pipe_opened != 2)
        {
            if (f)
                fclose(f);
            f = fopen("compare_pipe", "wb");
            pipe_opened = 2;
        }
        /*if(wait == 1 && reg_cop0[9] > 0x35000000) wait=0;
        if(wait) return;*/

        if (interpcore)
            fwrite(&interp_addr, 1, sizeof(int32_t), f);
        else
            fwrite(&PC->addr, 1, sizeof(int32_t), f);
        /*
        fwrite(reg, 32, sizeof(int64_t), f);
        fwrite(reg_cop0, 32, sizeof(int32_t), f);
        fwrite(reg_cop1_fgr_64, 32, sizeof(int64_t), f);
        */
        // fwrite(&rdram[0x31280/4], 1, sizeof(int32_t), f);
        /*fwrite(&FCR31, 4, 1, f);*/
    }
}
