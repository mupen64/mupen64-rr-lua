/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaDialog.hpp>
#include <lua/LuaManager.hpp>
#include <Common.Views/Hotkey.hpp>
#include <HotkeyUtils.hpp>

namespace LuaCore::Hotkey
{
static void push_hotkey(lua_State *L, const ::Hotkey &hotkey)
{
    lua_newtable(L);

    // COMPAT
    lua_pushstring(L, "key");
    lua_pushinteger(L, *HotkeyUtils::trigger_to_vk(hotkey.trigger));
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

    // COMPAT
    lua_pushstring(L, "assigned");
    lua_pushboolean(L, hotkey.is_assigned());
    lua_settable(L, -3);
}

static ::Hotkey check_hotkey(lua_State *L, int i)
{
    auto hotkey = ::Hotkey::make_empty();

    if (!lua_istable(L, i))
    {
        luaL_error(L, "Expected a table at argument %d", i);
        return hotkey;
    }

    // COMPAT
    lua_getfield(L, i, "key");
    const auto vk = luaL_opt(L, lua_tointeger, -1, 0);
    hotkey.trigger = *HotkeyUtils::vk_to_trigger(vk);
    lua_pop(L, 1);

    lua_getfield(L, i, "ctrl");
    hotkey.ctrl = luaL_opt(L, lua_toboolean, -1, false);
    lua_pop(L, 1);

    lua_getfield(L, i, "shift");
    hotkey.shift = luaL_opt(L, lua_toboolean, -1, false);
    lua_pop(L, 1);

    // COMPAT
    lua_getfield(L, i, "assigned");
    const auto assigned = luaL_opt(L, lua_toboolean, -1, true);
    if (!assigned) hotkey = ::Hotkey::make_unassigned();
    lua_pop(L, 1);

    return hotkey;
}

static int prompt(lua_State *L)
{
    WindowDisabler disabler(LuaDialog::hwnd());

    const auto caption = luaL_checkstlstring(L, 1);

    ::Hotkey hotkey = ::Hotkey::make_empty();

    const bool confirmed = HotkeyUtils::show_prompt(g_main_ctx.hwnd, caption, hotkey);

    if (!confirmed)
    {
        return 0;
    }

    push_hotkey(L, hotkey);
    return 1;
}
} // namespace LuaCore::Hotkey
