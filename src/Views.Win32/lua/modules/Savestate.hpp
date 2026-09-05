/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Main.hpp>
#include <lua/LuaManager.hpp>

namespace LuaCore::Savestate
{
static CoreSTJob lua_to_savestate_job(lua_State *l, const int i)
{
    const std::string str = lua_tostring(l, i);
    return str == "save" ? CoreSTJob::Save : CoreSTJob::Load;
}

static int do_file(lua_State *L)
{
    const std::filesystem::path path = lua_tostring(L, 1);
    const auto job = lua_to_savestate_job(L, 2);
    const auto callback = lua_tocallback(L, 3);
    const bool ignore_warnings = lua_toboolean(L, 4);

    g_main_ctx.core_ctx->vr_wait_increment();
    ThreadPool::submit_task([=] {
        g_main_ctx.core_ctx->st_do_file(
            path, job,
            [=](const CoreSTCallbackInfo &info, const std::vector<uint8_t> &buf) {
                g_main_ctx.dispatcher->invoke([=] {
                    if (!LuaManager::get_environment_for_state(L))
                    {
                        return;
                    }
                    lua_pushcallback(L, callback);
                    lua_pushinteger(L, static_cast<lua_Integer>(info.result));
                    lua_pushlstring(L, (const char *)buf.data(), buf.size());
                    lua_pcall(L, 2, 0, 0);
                });
            },
            ignore_warnings);
        g_main_ctx.core_ctx->vr_wait_decrement();
    });
    return 0;
}

static int do_slot(lua_State *L)
{
    const auto slot = lua_tointeger(L, 1) - 1;
    const auto job = lua_to_savestate_job(L, 2);
    const auto callback = lua_tocallback(L, 3);
    const bool ignore_warnings = lua_toboolean(L, 4);

    g_main_ctx.core_ctx->vr_wait_increment();
    ThreadPool::submit_task([=] {
        g_main_ctx.core_ctx->st_do_file(
            get_st_with_slot_path(slot), job,
            [=](const CoreSTCallbackInfo &info, const std::vector<uint8_t> &buf) {
                g_main_ctx.dispatcher->invoke([=] {
                    if (!LuaManager::get_environment_for_state(L))
                    {
                        return;
                    }
                    lua_pushcallback(L, callback);
                    lua_pushinteger(L, static_cast<lua_Integer>(info.result));
                    lua_pushlstring(L, (const char *)buf.data(), buf.size());
                    lua_pcall(L, 2, 0, 0);
                });
            },
            ignore_warnings);
        g_main_ctx.core_ctx->vr_wait_decrement();
    });
    return 0;
}

static int do_memory(lua_State *L)
{
    size_t buffer_len{};
    const auto buffer_str = lua_tolstring(L, 1, &buffer_len);
    const auto job = lua_to_savestate_job(L, 2);
    const auto callback = lua_tocallback(L, 3);
    const bool ignore_warnings = lua_toboolean(L, 4);

    g_main_ctx.core_ctx->vr_wait_increment();
    ThreadPool::submit_task([=] {
        const auto buffer = std::vector<uint8_t>(buffer_str, buffer_str + buffer_len);
        g_main_ctx.core_ctx->st_do_memory(
            buffer, job,
            [=](const CoreSTCallbackInfo &info, const std::vector<uint8_t> &buf) {
                g_main_ctx.dispatcher->invoke([=] {
                    if (!LuaManager::get_environment_for_state(L))
                    {
                        return;
                    }
                    lua_pushcallback(L, callback);
                    lua_pushinteger(L, static_cast<lua_Integer>(info.result));
                    lua_pushlstring(L, (const char *)buf.data(), buf.size());
                    lua_pcall(L, 2, 0, 0);
                });
            },
            ignore_warnings);
        g_main_ctx.core_ctx->vr_wait_decrement();
    });
    return 0;
}
} // namespace LuaCore::Savestate
