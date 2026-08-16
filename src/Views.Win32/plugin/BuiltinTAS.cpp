/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "BuiltinTAS.hpp"
#include <Loggers.hpp>
#include "../../Plugins.Win32.TASVideo/glN64.hpp"
#include <filesystem>

extern HINSTANCE g_inst;
extern HINSTANCE g_dll_handle;
extern std::filesystem::path g_dll_path;

namespace BuiltinTAS
{
void initialize_module_state()
{
    const auto instance = GetModuleHandle(nullptr);

    g_tas_ctx.hinst = instance;
    g_dll_handle = instance;
    g_dll_path = std::filesystem::path(instance ? "mupen64.exe" : "");
    ::g_inst = instance;
}

void initialize_input()
{
    initialize_module_state();
}

} // namespace BuiltinTAS
