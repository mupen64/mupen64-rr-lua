/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include "Config.hpp"
#include "HLE.hpp"

#define EXPORT __declspec(dllexport)
#undef CALL
#define CALL _cdecl

#define UCODE_MARIO (1)
#define UCODE_BANJO (2)
#define UCODE_ZELDA (3)

ZilmarExtSpec::RSPPluginInfo rsp;
static bool g_rsp_alive = false;
static void (*ABI[0x20])();
uint32_t inst1;
uint32_t inst2;
HINSTANCE g_instance;
std::filesystem::path g_config_path;

ZilmarExtSpec::ExtendedFuncs *g_ef{};

static int audio_ucode_detect_type(const OSTask_t *task)
{
    if (*(uint32_t *)(rsp.rdram + task->ucode_data + 0) != 0x1)
    {
        if (*(rsp.rdram + task->ucode_data + (0 ^ 3 - S8)) == 0xF) return 4;
        return 3;
    }

    if (*(uint32_t *)(rsp.rdram + task->ucode_data + 0x30) == 0xF0000F00) return 1;
    return 2;
}

static void ucode_load(const OSTask_t *task)
{
    const auto ucode_type = audio_ucode_detect_type(task);

    switch (ucode_type)
    {
    case UCODE_MARIO:
        memcpy(ABI, ABI1, sizeof(ABI[0]) * 0x20);
        break;
    case UCODE_BANJO:
        memcpy(ABI, ABI2, sizeof(ABI[0]) * 0x20);
        break;
    case UCODE_ZELDA:
        memcpy(ABI, ABI3, sizeof(ABI[0]) * 0x20);
        break;
    default:
        std::terminate();
    }
}

bool rsp_alive()
{
    return g_rsp_alive;
}

void on_rom_closed()
{
    memset(rsp.dmem, 0, 0x1000);
    memset(rsp.imem, 0, 0x1000);
    g_rsp_alive = false;
}

uint32_t do_rsp_cycles(uint32_t Cycles)
{
    g_rsp_alive = true;

    const auto task = (OSTask_t *)(rsp.dmem + 0xFC0);

    const auto effective_speed_mode = g_ef->get_effective_speed_mode();
    const auto skip_audio = effective_speed_mode == CoreSpeedMode::UltraFastForward;

    if (task->type == 1 && task->data_ptr != 0)
    {
        if (rsp.process_dlist_list) rsp.process_dlist_list();

        *rsp.sp_status_reg |= 0x0203;
        if ((*rsp.sp_status_reg & 0x40) != 0) *rsp.mi_intr_reg |= 0x1;

        *rsp.dpc_status_reg &= ~0x0002;
        return Cycles;
    }

    *rsp.sp_status_reg |= 0x203;
    if ((*rsp.sp_status_reg & 0x40) != 0)
    {
        *rsp.mi_intr_reg |= 0x1;
    }

    uint32_t sum = 0;

    if (task->ucode_size <= 0x1000)
    {
        for (uint32_t i = 0; i < task->ucode_size / 2; i++) sum += *(rsp.rdram + task->ucode + i);
    }
    else
    {
        for (uint32_t i = 0; i < 0x1000 / 2; i++) sum += *(rsp.imem + i);
    }

    if (task->ucode_size > 0x1000)
    {
        if (sum == 0x9E2)
        {
            // banjo tooie (U) boot code
            memcpy(rsp.imem + 0x120, rsp.rdram + 0x1e8, 0x1e8);
            for (int j = 0; j < 0xfc; j++)
                for (int i = 0; i < 8; i++)
                    *(rsp.rdram + ((0x2fb1f0 + (j * 0xff0) + i) ^ S8)) = *(rsp.imem + ((0x120 + (j * 8) + i) ^ S8));
            return Cycles;
        }

        if (sum == 0x9F2)
        {
            // banjo tooie (E) + zelda oot (E) boot code
            memcpy(rsp.imem + 0x120, rsp.rdram + 0x1e8, 0x1e8);
            for (int j = 0; j < 0xfc; j++)
                for (int i = 0; i < 8; i++)
                    *(rsp.rdram + ((0x2fb1f0 + (j * 0xff0) + i) ^ S8)) = *(rsp.imem + ((0x120 + (j * 8) + i) ^ S8));
            return Cycles;
        }

        goto unknown_task;
    }

    if (task->type == 2)
    {
        if (skip_audio) return Cycles;

        ucode_load(task);

        const auto p_alist = (uint32_t *)(rsp.rdram + task->data_ptr);
        for (uint32_t i = 0; i < task->data_size / 4; i += 2)
        {
            inst1 = p_alist[i];
            inst2 = p_alist[i + 1];
            const auto opcode = inst1 >> 24;
            const auto cmd = ABI[opcode];
            *g_ef->rcp_counter += 1;
            cmd();
        }

        return Cycles;
    }

    if (task->type == 4)
    {
        if (sum == 0x278)
        {
            *rsp.sp_status_reg |= 0x200;
            return Cycles;
        }
        if (sum == 0x2e4fc)
        {
            jpg_uncompress(task);
            return Cycles;
        }
    }

unknown_task:
    std::terminate();
    return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        g_instance = hinst;
        break;
    default:
        break;
    }

    return TRUE;
}

EXPORT void CALL DllAbout(void *hwnd)
{
    const auto msg = L"First-party TAS plugin for Mupen64."
                     L"\n"
                     L"TAS plugins are not to be distributed separately from Mupen64 and remain tied "
                     L"to one version of the emulator."
                     L"\n\n"
                     L"https://mupen64.com";
    MessageBox((HWND)hwnd, msg, L"About", MB_ICONINFORMATION | MB_OK);
}

EXPORT void CALL GetDllInfo(ZilmarExtSpec::PluginInfo *PluginInfo)
{
    PluginInfo->ver = 0x0101;
    PluginInfo->type = ZilmarExtSpec::PluginType::RSP;
    strcpy_s(PluginInfo->name, 100, IOUtils::to_utf8_string(PLUGIN_NAME).c_str());
    PluginInfo->unused_normal_memory = 1;
    PluginInfo->unused_byteswapped = 1;
    std::ranges::copy(IOUtils::to_utf8_string(CURRENT_VERSION), PluginInfo->target_version);
}

EXPORT void CALL InitiateRSP(ZilmarExtSpec::RSPPluginInfo Rsp_Info, uint32_t *CycleCount)
{
    g_ef = Rsp_Info.extended_funcs;
    g_config_path = ZilmarExtSpec::get_config_path(g_ef);
    rsp = Rsp_Info;
    config_load();
}

EXPORT void CALL RomClosed()
{
    on_rom_closed();
}

EXPORT uint32_t CALL DoRspCycles(uint32_t Cycles)
{
    return do_rsp_cycles(Cycles);
}
