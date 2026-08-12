/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <WinUtils.hpp>
#include <components/FilePicker.hpp>
#include <lua/LuaDialog.hpp>

namespace LuaCore::IOHelper
{
// IO
static int LuaFileDialog(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    BetterEmulationLock lock;
    WindowDisabler disabler(LuaDialog::hwnd());

    auto filter = std::string(luaL_checkstring(L, 1));
    const int32_t type = luaL_checkinteger(L, 2);

    std::filesystem::path path;

    if (type == 0)
    {
        path = FilePicker::show_open_dialog("o_lua_api", g_main_ctx.hwnd, filter);
    }
    else
    {
        path = FilePicker::show_save_dialog("s_lua_api", g_main_ctx.hwnd, filter);
    }

    lua_pushstring(L, path.string().c_str());
    return 1;
}
} // namespace LuaCore::IOHelper
