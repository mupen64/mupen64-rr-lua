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
static int add_breakpoint(lua_State *L)
{
    const uintptr_t address = luaL_checkinteger(L, 1);
    const auto callback = lua_optcallback(L, 2);

    const auto functor = [=](const core_dbg_cpu_state &state) {
        if (!callback || !LuaManager::get_environment_for_state(L)) return;
        lua_pushcallback(L, callback, false);
        lua_newtable(L);
        lua_pushinteger(L, state.address);
        lua_setfield(L, -2, "address");
        lua_pushinteger(L, state.opcode);
        lua_setfield(L, -2, "opcode");
        lua_pcall(L, 1, 0, 0);
    };

    const auto id = g_main_ctx.core_ctx->dbg_add_breakpoint(
        address, [=](const core_dbg_cpu_state &state) { g_main_ctx.dispatcher->invoke([&] { functor(state); }); });

    lua_pushinteger(L, id);
    return 1;
}

static int remove_breakpoint(lua_State *L)
{
    const CoreBreakpointId id = luaL_checkinteger(L, 1);
    g_main_ctx.core_ctx->dbg_remove_breakpoint(id);
    return 0;
}

static int pause(lua_State *L)
{
    g_main_ctx.core_ctx->dbg_set_resumed(false);
    return 0;
}

static int resume(lua_State *L)
{
    g_main_ctx.core_ctx->dbg_set_resumed(true);
    return 0;
}

static int step(lua_State *L)
{
    g_main_ctx.core_ctx->dbg_step();
    return 0;
}

} // namespace LuaCore::Debugger
