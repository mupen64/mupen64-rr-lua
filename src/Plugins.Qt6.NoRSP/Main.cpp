/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.h"
#include "core_plugin.h"
#include "core_types.h"
#include "mupapi.h"
#include <cstring>

EXPORT core_result CALL mup_init(const char *exe_dir, const core_plugin_extended_funcs *fwd_funcs)
{
    return Res_Ok;
}

EXPORT void CALL mup_drop()
{
}

EXPORT void CALL mup_get_info(core_plugin_info *info)
{
    info->ver = 0x0101;
    info->type = plugin_rsp;
    strncpy(info->name, PLUGIN_NAME, sizeof(info->name));
}