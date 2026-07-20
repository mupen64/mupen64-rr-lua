/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include "Config.hpp"
#include "HLE.hpp"

#define UCODE_MARIO (1)
#define UCODE_BANJO (2)
#define UCODE_ZELDA (3)

M64RRSpec::PluginInit* rsp;
static bool g_rsp_alive = false;
static void (*ABI[0x20])();
uint32_t inst1;
uint32_t inst2;
std::filesystem::path g_config_path;

M64RRSpec::ExtendedFuncs *g_ef{};

static int audio_ucode_detect_type(const OSTask_t *task)
{
    if (*(uint32_t *)(rsp->rdram + task->ucode_data + 0) != 0x1)
    {
        if (*(rsp->rdram + task->ucode_data + (0 ^ 3 - S8)) == 0xF) return 4;
        return 3;
    }

    if (*(uint32_t *)(rsp->rdram + task->ucode_data + 0x30) == 0xF0000F00) return 1;
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
    memset(rsp->dmem, 0, 0x1000);
    memset(rsp->imem, 0, 0x1000);
    g_rsp_alive = false;
}

uint32_t do_rsp_cycles(uint32_t Cycles)
{
    g_rsp_alive = true;

    const auto task = (OSTask_t *)(rsp->dmem + 0xFC0);

    const auto effective_speed_mode = g_ef->get_effective_speed_mode();
    const auto skip_audio = effective_speed_mode == CoreSpeedMode::UltraFastForward;

    if (task->type == 1 && task->data_ptr != 0)
    {
        rsp->process_dlist();

        *rsp->sp_status_reg |= 0x0203;
        if ((*rsp->sp_status_reg & 0x40) != 0) *rsp->mi_intr_reg |= 0x1;

        *rsp->dpc_status_reg &= ~0x0002;
        return Cycles;
    }

    *rsp->sp_status_reg |= 0x203;
    if ((*rsp->sp_status_reg & 0x40) != 0)
    {
        *rsp->mi_intr_reg |= 0x1;
    }

    uint32_t sum = 0;

    if (task->ucode_size <= 0x1000)
    {
        for (uint32_t i = 0; i < task->ucode_size / 2; i++) sum += *(rsp->rdram + task->ucode + i);
    }
    else
    {
        for (uint32_t i = 0; i < 0x1000 / 2; i++) sum += *(rsp->imem + i);
    }

    if (task->ucode_size > 0x1000)
    {
        if (sum == 0x9E2)
        {
            // banjo tooie (U) boot code
            memcpy(rsp->imem + 0x120, rsp->rdram + 0x1e8, 0x1e8);
            for (int j = 0; j < 0xfc; j++)
                for (int i = 0; i < 8; i++)
                    *(rsp->rdram + ((0x2fb1f0 + (j * 0xff0) + i) ^ S8)) = *(rsp->imem + ((0x120 + (j * 8) + i) ^ S8));
            return Cycles;
        }

        if (sum == 0x9F2)
        {
            // banjo tooie (E) + zelda oot (E) boot code
            memcpy(rsp->imem + 0x120, rsp->rdram + 0x1e8, 0x1e8);
            for (int j = 0; j < 0xfc; j++)
                for (int i = 0; i < 8; i++)
                    *(rsp->rdram + ((0x2fb1f0 + (j * 0xff0) + i) ^ S8)) = *(rsp->imem + ((0x120 + (j * 8) + i) ^ S8));
            return Cycles;
        }

        goto unknown_task;
    }

    if (task->type == 2)
    {
        if (skip_audio) return Cycles;

        ucode_load(task);

        const auto p_alist = (uint32_t *)(rsp->rdram + task->data_ptr);
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
            *rsp->sp_status_reg |= 0x200;
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

EXPORT void CALL M64RRGetMetadata(M64RRSpec::PluginMetadata *metadata)
{
    metadata->type = M64RRSpec::PluginType::RSP;

    const auto name = IOUtils::to_utf8_string(PLUGIN_NAME);
    const auto description = "First-party TAS plugin for Mupen64."
                             "\n"
                             "TAS plugins are not to be distributed separately from Mupen64 and remain tied "
                             "to one version of the emulator."
                             "\n\n"
                             "https://mupen64.com";
    const auto target_version = IOUtils::to_utf8_string(CURRENT_VERSION);

    auto result = std::format_to_n(metadata->name, sizeof(metadata->name) - 1, "{}", name);
    metadata->name[result.size] = '\0';

    result = std::format_to_n(metadata->description, sizeof(metadata->description) - 1, "{}", description);
    metadata->description[result.size] = '\0';

    result = std::format_to_n(metadata->target_version, sizeof(metadata->target_version) - 1, "{}", target_version);
    metadata->target_version[result.size] = '\0';
}

EXPORT void CALL M64RRInitiate(M64RRSpec::PluginInit *init)
{
    rsp = init;
    g_ef = rsp->ef;
    g_config_path = ZESpec::get_config_path(g_ef);
}

EXPORT void CALL M64RRRomOpened()
{
    config_load();
}

EXPORT void CALL M64RRRomClosed()
{
    on_rom_closed();
}

EXPORT void CALL M64RRDoRSPCycles(uint8_t cycles)
{
    do_rsp_cycles(cycles);
}
