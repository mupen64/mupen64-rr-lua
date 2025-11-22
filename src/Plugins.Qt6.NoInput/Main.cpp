/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Main.h"

// PlatformService platform_service;

// ReSharper disable once CppInconsistentNaming
core_result mup_init(const char *exe_dir, const mup_core_functions *fwd_funcs) {
    return Res_Ok;
}

void mup_drop() {

}

void mup_get_info(core_plugin_info *info) {
    info->ver = 0x0101;
    info->type = plugin_audio;
    strncpy(info->name, PLUGIN_NAME, sizeof(info->name));
}

EXPORT void CALL InitiateControllers(core_input_info ControlInfo)
{
    ControlInfo.controllers[0].Present = true;
}