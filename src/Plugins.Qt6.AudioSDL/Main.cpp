/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include "SDLBackend.hpp"
#include "core_plugin.h"
#include "core_types.h"
#include "mupapi.h"
#include <optional>
#include <string>

const core_plugin_extended_funcs* g_fwd_funcs = nullptr;
std::optional<core_audio_info> g_core_info;

static std::optional<AudioSDL::SDLBackend> g_backend = std::nullopt;

namespace {
static uint32_t compute_sample_rate(uint32_t system_type, uint32_t dacrate) {
    uint32_t vi_clock = 0;
    switch (system_type) {
        case sys_ntsc:
            vi_clock = 48681812;
            break;
        case sys_pal:
            vi_clock = 49656530;
            break;
        default:
            // fallback to NTSC
            vi_clock = 48681812;
            break;
    }

    return vi_clock / (dacrate + 1);
}
}

EXPORT core_result CALL mup_init(const char *plugin_dir, const core_plugin_extended_funcs *fwd_funcs)
{
    g_fwd_funcs = fwd_funcs;
    return Res_Ok;
}

EXPORT void CALL mup_drop()
{
    g_fwd_funcs = nullptr;
}

EXPORT void CALL mup_get_info(core_plugin_info *info)
{
    info->ver = 0x0101;
    info->type = plugin_audio;
    strncpy(info->name, PLUGIN_NAME, sizeof(info->name));
}

EXPORT void CALL mup_rom_opened()
{
}

EXPORT void CALL mup_rom_closed()
{
    g_backend.reset();
}

EXPORT void CALL mupa_init(core_audio_info core_info)
{
    g_core_info.emplace(core_info);
    g_backend.emplace();
}

EXPORT void CALL mupa_ai_dacrate_changed(int32_t system_type)
{
    if (!g_backend || !g_core_info)
        return;

    uint32_t sample_rate = compute_sample_rate(system_type, *g_core_info->ai_dacrate_reg);
    g_backend->set_sample_rate(sample_rate);
}

EXPORT void CALL mupa_ai_len_changed()
{
    if (!g_backend || !g_core_info)
        return;

    uint32_t addr = *g_core_info->ai_dram_addr_reg & 0x00FF'FFF8;
    uint32_t len = *g_core_info->ai_len_reg & 0x0003'FFF8;

    g_backend->push_samples(g_core_info->rdram + addr, len);
    g_backend->sync_audio();
}

EXPORT uint32_t CALL mupa_ai_read_length()
{
    return 0;
}

EXPORT void mupa_process_a_list()
{
}

EXPORT void mupa_ai_update(int32_t wait)
{
}