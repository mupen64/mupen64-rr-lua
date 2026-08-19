/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing functionality related to the Lua function registry.
 */
namespace LuaRegistry
{
/**
 * \brief Registers the standard Lua functions along with Mupen's functions to the specified state.
 */
void register_functions(lua_State *L);
} // namespace LuaRegistry
