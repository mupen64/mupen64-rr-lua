/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <features/FilePicker.h>

namespace LuaCore::IOHelper
{
    // IO
    static int LuaFileDialog(lua_State* L)
    {
        auto lua = get_lua_class(L);

        BetterEmulationLock lock;

        auto filter = string_to_wstring(std::string(luaL_checkstring(L, 1)));
        const int32_t type = luaL_checkinteger(L, 2);

        std::wstring path;

        EnableWindow(lua->hwnd, FALSE);
        if (type == 0)
        {
            path = show_persistent_open_dialog(L"o_lua_api", g_main_hwnd, filter);
        }
        else
        {
            path = show_persistent_save_dialog(L"s_lua_api", g_main_hwnd, filter);
        }
        EnableWindow(lua->hwnd, TRUE);
        lua_pushstring(L, wstring_to_string(path).c_str());
        return 1;
    }
} // namespace LuaCore::IOHelper
