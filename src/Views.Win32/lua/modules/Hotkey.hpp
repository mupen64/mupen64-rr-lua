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
static void push_trigger(lua_State *L, const ::Hotkey::Trigger &trigger)
{
    lua_newtable(L);

    if (std::holds_alternative<std::monostate>(trigger))
    {
        lua_pushstring(L, "none");
    }
    else if (std::holds_alternative<::Hotkey::KeyCode>(trigger))
    {
        lua_pushstring(L, "keycode");
        lua_pushinteger(L, std::get<::Hotkey::KeyCode>(trigger).get());
        lua_setfield(L, -2, "value");
    }
    else
    {
        lua_pushstring(L, "mousebutton");
        lua_pushinteger(L, std::get<::Hotkey::MouseButton>(trigger).get());
        lua_setfield(L, -2, "value");
    }

    lua_setfield(L, -2, "type");
}

static void push_hotkey(lua_State *L, const ::Hotkey &hotkey)
{
    lua_newtable(L);

    push_trigger(L, hotkey.trigger);
    lua_setfield(L, -2, "trigger");

    lua_pushboolean(L, hotkey.ctrl);
    lua_setfield(L, -2, "ctrl");

    lua_pushboolean(L, hotkey.shift);
    lua_setfield(L, -2, "shift");

    lua_pushboolean(L, hotkey.alt);
    lua_setfield(L, -2, "alt");
}

static ::Hotkey::Trigger check_trigger(lua_State *L, int i)
{
    luaL_checktype(L, i, LUA_TTABLE);

    lua_getfield(L, i, "type");
    const std::string type = luaL_checkstring(L, -1);
    lua_pop(L, 1);

    if (type == "none") return std::monostate{};
    if (type != "keycode" && type != "mousebutton")
    {
        luaL_error(L, "Unknown hotkey trigger type: %s", type.c_str());
        std::unreachable();
    }

    lua_getfield(L, i, "value");
    const auto value = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    if (type == "keycode") return ::Hotkey::KeyCode(static_cast<SDL_Keycode>(value));
    return ::Hotkey::MouseButton(static_cast<SDL_MouseButtonFlags>(value));
}

static ::Hotkey check_hotkey(lua_State *L, int i)
{
    luaL_checktype(L, i, LUA_TTABLE);

    lua_getfield(L, i, "trigger");
    ::Hotkey hotkey(check_trigger(L, -1));
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
