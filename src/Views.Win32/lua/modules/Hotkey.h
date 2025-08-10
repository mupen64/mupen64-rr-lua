/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <components/LuaDialog.h>
#include <lua/LuaManager.h>

namespace LuaCore::Hotkey
{
    static void push_hotkey(lua_State* L, const ::Hotkey::t_hotkey& hotkey)
    {
        lua_newtable(L);

        lua_pushstring(L, "key");
        lua_pushinteger(L, hotkey.key);
        lua_settable(L, -3);

        lua_pushstring(L, "ctrl");
        lua_pushboolean(L, hotkey.ctrl);
        lua_settable(L, -3);

        lua_pushstring(L, "shift");
        lua_pushboolean(L, hotkey.shift);
        lua_settable(L, -3);

        lua_pushstring(L, "alt");
        lua_pushboolean(L, hotkey.alt);
        lua_settable(L, -3);
    }

    static ::Hotkey::t_hotkey check_hotkey(lua_State* L, int i)
    {
        ::Hotkey::t_hotkey hotkey{};

        if (!lua_istable(L, i))
        {
            luaL_error(L, "Expected a table at argument %d", i);
            return hotkey;
        }

        lua_getfield(L, i, "key");
        hotkey.key = luaL_opt(L, lua_tointeger, -1, 0);
        lua_pop(L, 1);

        lua_getfield(L, i, "ctrl");
        hotkey.ctrl = luaL_opt(L, lua_toboolean, -1, false);
        lua_pop(L, 1);

        lua_getfield(L, i, "shift");
        hotkey.shift = luaL_opt(L, lua_toboolean, -1, false);
        lua_pop(L, 1);

        lua_getfield(L, i, "alt");
        hotkey.alt = luaL_opt(L, lua_toboolean, -1, false);
        lua_pop(L, 1);

        return hotkey;
    }

    static int prompt(lua_State* L)
    {
        WindowDisabler disabler(LuaDialog::hwnd());

        const auto caption = luaL_checkwstring(L, 1);

        ::Hotkey::t_hotkey hotkey{};

        const bool confirmed = ::Hotkey::show_prompt(g_main_hwnd, caption, hotkey);

        if (!confirmed)
        {
            return 0;
        }

        push_hotkey(L, hotkey);
        return 1;
    }
} // namespace LuaCore::Hotkey
