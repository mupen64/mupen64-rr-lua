/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "stdafx.h"
#include <ThreadPool.h>

namespace LuaCore::Movie
{
static int play(lua_State *L)
{
    const auto path = lua_tostring(L, 1);
    if (!path)
    {
        lua_pushinteger(L, VCR_BadFile);
        return 1;
    }

    g_config.core.vcr_readonly = true;
    Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)g_config.core.vcr_readonly);
    ThreadPool::submit_task([=] { g_main_ctx.core_ctx->vcr_start_playback(path); });

    lua_pushinteger(L, Res_Ok);
    return 1;
}

static int stop(lua_State *L)
{
    const auto result = g_main_ctx.core_ctx->vcr_stop_all();
    lua_pushinteger(L, result);
    return 1;
}

static int GetMovieFilename(lua_State *L)
{
    if (g_main_ctx.core_ctx->vcr_get_task() == task_idle)
    {
        luaL_error(L, "No movie is currently playing");
        lua_pushstring(L, "");
    }
    else
    {
        lua_pushstring(L, g_main_ctx.core_ctx->vcr_get_path().string().c_str());
    }
    return 1;
}

static int GetVCRReadOnly(lua_State *L)
{
    lua_pushboolean(L, g_config.core.vcr_readonly);
    return 1;
}

static int SetVCRReadOnly(lua_State *L)
{
    g_config.core.vcr_readonly = lua_toboolean(L, 1);
    Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)g_config.core.vcr_readonly);
    return 0;
}

static int begin_seek(lua_State *L)
{
    auto str = std::string(lua_tostring(L, 1));
    bool pause_at_end = lua_toboolean(L, 2);

    lua_pushinteger(L, static_cast<int32_t>(g_main_ctx.core_ctx->vcr_begin_seek(str, pause_at_end)));
    return 1;
}

static int stop_seek(lua_State *L)
{
    g_main_ctx.core_ctx->vcr_stop_seek();
    return 0;
}

static int is_seeking(lua_State *L)
{
    lua_pushboolean(L, g_main_ctx.core_ctx->vcr_is_seeking());
    return 1;
}

static int get_seek_completion(lua_State *L)
{
    const core_vcr_seek_info info = g_main_ctx.core_ctx->vcr_get_seek_info();

    lua_newtable(L);
    lua_pushinteger(L, info.current_sample);
    lua_rawseti(L, -2, 1);
    lua_pushinteger(L, info.seek_target_sample);
    lua_rawseti(L, -2, 2);

    return 1;
}

/**
 * Begins a warp modify operation.
 *
 * Accepts a table of inputs as:
 *
 * {
 *   {A = true, B = true, Z = true },
 *   {A = true, B = true, Start = true },
 * }
 *
 * Inputs which are not specified stay at their default value, 0.
 *
 * \param L
 * \return
 */
static int begin_warp_modify(lua_State *L)
{
    std::vector<core_buttons> inputs;

    luaL_checktype(L, 1, LUA_TTABLE);
    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        luaL_checktype(L, -1, LUA_TTABLE);
        lua_pushnil(L);
        core_buttons buttons{};
        while (lua_next(L, -2))
        {
            std::string key = lua_tostring(L, -2);
            if (lua_toboolean(L, -1))
            {
                if (key == "right")
                {
                    buttons.dr = 1;
                }
                else if (key == "left")
                {
                    buttons.dl = 1;
                }
                else if (key == "down")
                {
                    buttons.dd = 1;
                }
                else if (key == "up")
                {
                    buttons.du = 1;
                }
                else if (key == "start")
                {
                    buttons.start = 1;
                }
                else if (key == "Z")
                {
                    buttons.z = 1;
                }
                else if (key == "B")
                {
                    buttons.b = 1;
                }
                else if (key == "A")
                {
                    buttons.a = 1;
                }
                else if (key == "Cright")
                {
                    buttons.cr = 1;
                }
                else if (key == "Cleft")
                {
                    buttons.cl = 1;
                }
                else if (key == "Cdown")
                {
                    buttons.cd = 1;
                }
                else if (key == "Cup")
                {
                    buttons.cu = 1;
                }
                else if (key == "R")
                {
                    buttons.r = 1;
                }
                else if (key == "L")
                {
                    buttons.l = 1;
                }
            }
            if (lua_tointeger(L, -1))
            {
                auto value = luaL_checkinteger(L, -1);
                if (key == "X")
                {
                    buttons.x = value;
                }
                if (key == "Y")
                {
                    buttons.y = value;
                }
            }
            lua_pop(L, 1);
        }
        inputs.push_back(buttons);
        lua_pop(L, 1);
    }

    auto result = g_main_ctx.core_ctx->vcr_begin_warp_modify(inputs);

    lua_pushinteger(L, static_cast<int32_t>(result));
    return 1;
}
} // namespace LuaCore::Movie
