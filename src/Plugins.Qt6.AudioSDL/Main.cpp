/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.hpp"
#include "core_plugin.h"
#include "core_types.h"
#include "mupapi.h"
#include <optional>
#include <string>

const core_plugin_extended_funcs* g_fwd_funcs = nullptr;
std::optional<core_audio_info> g_core_info;

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
}

EXPORT void CALL mupa_init(core_audio_info core_info)
{
    g_core_info.emplace(std::move(core_info));
}

EXPORT void CALL mupa_ai_dacrate_changed(int32_t system_type)
{
}

EXPORT void CALL mupa_ai_len_changed()
{
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