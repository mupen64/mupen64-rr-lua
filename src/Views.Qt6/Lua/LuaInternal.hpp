/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

// Private header shared by the Lua API modules (LuaGlobal, LuaEmu, LuaMemory).
// Not part of the public interface of the Lua library.

#include "LuaInstance.hpp"

#include <string_view>

namespace LuaInternal
{

/// Retrieves the LuaInstance owning the given Lua state.
LuaInstance *getInstance(lua_State *L);

/// Registers the global functions (print, console).
void registerGlobalAPIs(lua_State *L);

/// Registers the `emu` table.
void registerEmuAPIs(lua_State *L);

/// Registers the `memory` table.
void registerMemoryAPIs(lua_State *L);

/// Registers the `joypad` table.
void registerJoypadAPIs(lua_State *L);

/// Registers the `wgui` and `painter` tables.
void registerGuiAPIs(lua_State *L);

/**
 * Executes a std::string_view, much like `luaL_dostring`.
 */
inline int doStringView(lua_State *L, std::string_view str)
{
    // analogous to luaL_loadstring() || lua_pcall()
    return luaL_loadbuffer(L, str.data(), str.size(), nullptr) || lua_pcall(L, 0, LUA_MULTRET, 0);
}

} // namespace LuaInternal
