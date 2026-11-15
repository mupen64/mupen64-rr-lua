/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

// Private header shared by the Lua API modules (LuaGlobal, LuaEmu, LuaMemory).
// Not part of the public interface of the Lua library.

#include "LuaInstance.hpp"

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

} // namespace LuaInternal
