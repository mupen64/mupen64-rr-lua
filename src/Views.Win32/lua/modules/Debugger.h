/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaRenderer.h>
#include <lua/LuaManager.h>

namespace LuaCore::Debugger
{

static void push_cpu_state(lua_State *L, const core_dbg_cpu_state &state)
{
    lua_newtable(L);
    lua_pushinteger(L, state.address);
    lua_setfield(L, -2, "address");
    lua_pushinteger(L, state.opcode);
    lua_setfield(L, -2, "opcode");
}

static int add_breakpoint(lua_State *L)
{
    const auto env = LuaManager::get_environment_for_state(L);

    const uintptr_t address = luaL_checkinteger(L, 1);
    const auto callback = lua_optcallback(L, 2);

    const auto functor = [=](const core_dbg_cpu_state &state) {
        if (!callback || !LuaManager::get_environment_for_state(L)) return;
        lua_pushcallback(L, callback, false);
        push_cpu_state(L, state);
        lua_pcall(L, 1, 0, 0);
    };

    const auto id = g_main_ctx.core_ctx->dbg_add_breakpoint(
        address, [=](const core_dbg_cpu_state &state) { g_main_ctx.dispatcher->invoke([=] { functor(state); }); });

    env->active_breakpoints.emplace_back(std::make_pair(id, callback));

    lua_pushinteger(L, id);
    return 1;
}

static int remove_breakpoint(lua_State *L)
{
    const auto env = LuaManager::get_environment_for_state(L);

    const CoreBreakpointId id = luaL_checkinteger(L, 1);

    g_main_ctx.core_ctx->dbg_remove_breakpoint(id);

    const auto it = std::find_if(env->active_breakpoints.begin(), env->active_breakpoints.end(),
                                 [&](const std::pair<CoreBreakpointId, uintptr_t *> &v) { return v.first == id; });
    if (it != env->active_breakpoints.end())
    {
        lua_freecallback(L, it->second);
        env->active_breakpoints.erase(it);
    }

    return 0;
}

static int disassemble(lua_State *L)
{
    lua_getfield(L, 1, "address");
    const uintptr_t address = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "opcode");
    const uint32_t opcode = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    const auto str = g_main_ctx.core_ctx->dbg_disassemble({address, opcode});

    lua_pushstring(L, str.c_str());
    return 1;
}

} // namespace LuaCore::Debugger
