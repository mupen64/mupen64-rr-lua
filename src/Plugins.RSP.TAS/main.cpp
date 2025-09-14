/*
 * Copyright (c) 2025, hacktarux-azimer-rsp-hle maintainers, contributors, and original authors (Hacktarux, Azimer).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "win.h"
#include "Config.h"
#include "hle.h"

#define EXPORT __declspec(dllexport)
#define CALL _cdecl

core_rsp_info rsp;

// Whether RSP has been called since last ROM close
bool g_rsp_alive = false;

void* audio_plugin = nullptr;
extern void (*g_processAList)();

#define UCODE_MARIO (1)
#define UCODE_BANJO (2)
#define UCODE_ZELDA (3)

extern void (*ABI1[0x20])();
extern void (*ABI2[0x20])();
extern void (*ABI3[0x20])();

void (*ABI[0x20])();

uint32_t inst1, inst2;

void (*g_audio_ucode_func)() = nullptr;

/**
 * \brief Loads the audio plugin's globals
 * \param path Path to an audio plugin dll
 * \returns Handle to the audio plugin, or nullptr.
 */
void* plugin_load(const std::filesystem::path& path);

/**
 * \brief Handles unknown RSP tasks.
 */
void handle_unknown_task(const OSTask_t* task, uint32_t sum);

void audio_ucode_mario()
{
    memcpy(ABI, ABI1, sizeof(ABI[0]) * 0x20);
}

void audio_ucode_banjo()
{
    memcpy(ABI, ABI2, sizeof(ABI[0]) * 0x20);
}

void audio_ucode_zelda()
{
    memcpy(ABI, ABI3, sizeof(ABI[0]) * 0x20);
}


int audio_ucode_detect_type(const OSTask_t* task)
{
    if (*(unsigned long*)(rsp.rdram + task->ucode_data + 0) != 0x1)
    {
        if (*(rsp.rdram + task->ucode_data + (0 ^ 3 - S8)) == 0xF)
            return 4;
        return 3;
    }

    if (*(unsigned long*)(rsp.rdram + task->ucode_data + 0x30) == 0xF0000F00)
        return 1;
    return 2;
}

void audio_ucode_verify_cache(const OSTask_t* task)
{
    // In debug mode, we want to verify that the ucode type hasn't changed
    const auto ucode_type = audio_ucode_detect_type(task);

    switch (ucode_type)
    {
    case UCODE_MARIO:
        assert(g_audio_ucode_func == audio_ucode_mario);
        break;
    case UCODE_BANJO:
        assert(g_audio_ucode_func == audio_ucode_banjo);
        break;
    case UCODE_ZELDA:
        assert(g_audio_ucode_func == audio_ucode_zelda);
        break;
    default:
        break;
    }
}

int audio_ucode(OSTask_t* task)
{
    if (!g_audio_ucode_func)
    {
        const auto ucode_type = audio_ucode_detect_type(task);

        printf("[RSP] Detected ucode type: %d\n", ucode_type);

        switch (ucode_type)
        {
        case UCODE_MARIO:
            g_audio_ucode_func = audio_ucode_mario;
            break;
        case UCODE_BANJO:
            g_audio_ucode_func = audio_ucode_banjo;
            break;
        case UCODE_ZELDA:
            g_audio_ucode_func = audio_ucode_zelda;
            break;
        default:
            printf("[RSP] Unknown ucode type: %d\n", ucode_type);
            return -1;
        }
    }

    if (config.ucode_cache_verify)
    {
        audio_ucode_verify_cache(task);
    }

    g_audio_ucode_func();

    const auto p_alist = (unsigned long*)(rsp.rdram + task->data_ptr);

    for (unsigned int i = 0; i < task->data_size / 4; i += 2)
    {
        inst1 = p_alist[i];
        inst2 = p_alist[i + 1];
        ABI[inst1 >> 24]();
    }

    return 0;
}

EXPORT uint32_t CALL DoRspCycles(uint32_t Cycles)
{
    OSTask_t* task = (OSTask_t*)(rsp.dmem + 0xFC0);
    unsigned int i, sum = 0;

    g_rsp_alive = true;

    // For first-time initialization of audio plugin
    // I think it's safe to keep the plugin loaded across emulation starts...
    if (config.audio_external && !audio_plugin)
    {
        audio_plugin = plugin_load(config.audio_path);
    }


    if (task->type == 1 && task->data_ptr != 0 && config.graphics_hle)
    {
        if (rsp.process_dlist_list)
        {
            rsp.process_dlist_list();
        }
        *rsp.sp_status_reg |= 0x0203;
        if ((*rsp.sp_status_reg & 0x40) != 0)
        {
            *rsp.mi_intr_reg |= 0x1;
            rsp.check_interrupts();
        }

        *rsp.dpc_status_reg &= ~0x0002;
        return Cycles;
    }

    if (task->type == 2 && config.audio_hle)
    {
        if (config.audio_external)
            g_processAList();
        else if (rsp.process_alist_list != NULL)
        {
            rsp.process_alist_list();
        }
        *rsp.sp_status_reg |= 0x0203;
        if ((*rsp.sp_status_reg & 0x40) != 0)
        {
            *rsp.mi_intr_reg |= 0x1;
            rsp.check_interrupts();
        }
        return Cycles;
    }

    if (task->type == 7)
    {
        rsp.show_cfb();
    }

    *rsp.sp_status_reg |= 0x203;
    if ((*rsp.sp_status_reg & 0x40) != 0)
    {
        *rsp.mi_intr_reg |= 0x1;
        rsp.check_interrupts();
    }

    if (task->ucode_size <= 0x1000)
    {
        for (i = 0; i < task->ucode_size / 2; i++)
            sum += *(rsp.rdram + task->ucode + i);
    }
    else
        for (i = 0; i < 0x1000 / 2; i++)
        {
            sum += *(rsp.imem + i);
        }


    if (task->ucode_size > 0x1000)
    {
        switch (sum)
        {
        case 0x9E2: // banjo tooie (U) boot code
            {
                int i, j;
                memcpy(rsp.imem + 0x120, rsp.rdram + 0x1e8, 0x1e8);
                for (j = 0; j < 0xfc; j++)
                    for (i = 0; i < 8; i++)
                        *(rsp.rdram + (0x2fb1f0 + j * 0xff0 + i ^ S8)) = *(rsp.imem + (0x120 + j * 8 + i ^ S8));
            }
            return Cycles;
        case 0x9F2: // banjo tooie (E) + zelda oot (E) boot code
            {
                int i, j;
                memcpy(rsp.imem + 0x120, rsp.rdram + 0x1e8, 0x1e8);
                for (j = 0; j < 0xfc; j++)
                    for (i = 0; i < 8; i++)
                        *(rsp.rdram + (0x2fb1f0 + j * 0xff0 + i ^ S8)) = *(rsp.imem + (0x120 + j * 8 + i ^ S8));
            }
            return Cycles;
        }
    }
    else
    {
        switch (task->type)
        {
        case 2: // audio
            if (audio_ucode(task) == 0)
                return Cycles;
            break;
        case 4: // jpeg
            switch (sum)
            {
            case 0x278: // used by zelda during boot
                *rsp.sp_status_reg |= 0x200;
                return Cycles;
            case 0x2e4fc: // uncompress
                jpg_uncompress(task);
                return Cycles;
            default:
                MessageBox(NULL, std::format(L"unknown jpeg: sum: {}", sum).c_str(), L"Error", MB_OK | MB_ICONERROR);
                break;
            }
            break;
        }
    }

    handle_unknown_task(task, sum);

    return Cycles;
}

EXPORT void CALL CloseDLL(void)
{
}

EXPORT void CALL GetDllInfo(core_plugin_info* PluginInfo)
{
    PluginInfo->ver = 0x0101;
    PluginInfo->type = (int16_t)plugin_rsp;
    strncpy(PluginInfo->name, PLUGIN_NAME, std::size(PluginInfo->name));
    PluginInfo->unused_normal_memory = 1;
    PluginInfo->unused_byteswapped = 1;
}

EXPORT void CALL InitiateRSP(core_rsp_info Rsp_Info, uint32_t* CycleCount)
{
    rsp = Rsp_Info;
}

EXPORT void CALL RomOpen(void)
{
    g_config_readonly = true;
}

EXPORT void CALL RomClosed(void)
{
    memset(rsp.dmem, 0, 0x1000);
    memset(rsp.imem, 0, 0x1000);

    g_audio_ucode_func = nullptr;
    g_rsp_alive = false;
    g_config_readonly = false;
}
