/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.h"
#include "core_plugin.h"
#include "core_types.h"
#include "mupapi.h"

EXPORT core_result CALL mup_init(const char *exe_dir, const core_plugin_extended_funcs *fwd_funcs)
{
}

EXPORT void CALL mup_drop()
{
}

EXPORT void CALL mup_get_info(core_plugin_info *info)
{
}

EXPORT void CALL mup_rom_opened()
{
}

EXPORT void CALL mup_rom_closed()
{
}

EXPORT void CALL mupa_init(core_audio_info core_info)
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