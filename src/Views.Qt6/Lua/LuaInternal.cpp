/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaInternal.hpp"
#include "LuaInstance.hpp"

#include <lua.hpp>

namespace LuaInternal
{

LuaInstance *getInstance(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "__lua_instance");
    auto *inst = static_cast<LuaInstance *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return inst;
}

} // namespace LuaInternal
