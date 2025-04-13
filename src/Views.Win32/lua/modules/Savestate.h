/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaConsole.h>

namespace LuaCore::Savestate
{
    static int SaveFileSavestate(lua_State* L)
    {
        const std::string path = lua_tostring(L, 1);

        core_vr_wait_increment();
        AsyncExecutor::invoke_async([=] {
            core_vr_wait_decrement();
            core_st_do_file(path, core_st_job_save, nullptr, false);
        });

        return 0;
    }

    static int LoadFileSavestate(lua_State* L)
    {
        const std::string path = lua_tostring(L, 1);

        core_vr_wait_increment();
        AsyncExecutor::invoke_async([=] {
            core_vr_wait_decrement();
            core_st_do_file(path, core_st_job_load, nullptr, false);
        });

        return 0;
    }

    static int do_file(lua_State* L)
    {
        const std::filesystem::path path = lua_tostring(L, 1);
        const auto job = static_cast<core_st_job>(lua_tointeger(L, 2));
        const auto callback = lua_tocallback(L, 3);
        const bool ignore_warnings = lua_toboolean(L, 4);

        core_vr_wait_increment();
        AsyncExecutor::invoke_async([=] {
            core_vr_wait_decrement();
            core_st_do_file(path, job, [=](const auto result, const std::vector<uint8_t>& buf) {
                g_main_window_dispatcher->invoke([=] {
                    if (!get_lua_class(L))
                    {
                        return;
                    }
                    lua_pushcallback(L, callback);
                    lua_pushinteger(L, result);
                    lua_pushlstring(L, (const char*)buf.data(), buf.size());
                    lua_pcall(L, 2, 0, 0);
                });
            },
                            ignore_warnings);
        });
        return 0;
    }

    static int do_slot(lua_State* L)
    {
        return 0;
    }

    static int do_memory(lua_State* L)
    {
        return 0;
    }
} // namespace LuaCore::Savestate
