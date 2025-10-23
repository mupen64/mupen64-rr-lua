/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <ViewHelpers.h>
#include <components/FilePicker.h>
#include <components/LuaDialog.h>

namespace LuaCore::IOHelper
{
// IO
static int LuaFileDialog(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    BetterEmulationLock lock;
    WindowDisabler disabler(LuaDialog::hwnd());

    auto filter = IOUtils::to_wide_string(std::string(luaL_checkstring(L, 1)));
    const int32_t type = luaL_checkinteger(L, 2);

    std::wstring path;

    if (type == 0)
    {
        path = FilePicker::show_open_dialog(L"o_lua_api", g_main_ctx.hwnd, filter);
    }
    else
    {
        path = FilePicker::show_save_dialog(L"s_lua_api", g_main_ctx.hwnd, filter);
    }

    lua_pushstring(L, IOUtils::to_utf8_string(path).c_str());
    return 1;
}
} // namespace LuaCore::IOHelper
