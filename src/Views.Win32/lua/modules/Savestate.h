/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Main.h>
#include <lua/LuaConsole.h>

namespace LuaCore::Savestate
{
    static int SaveFileSavestate(lua_State* L)
    {
        const std::string path = lua_tostring(L, 1);

        core_vr_wait_increment();
        ThreadPool::submit_task([=] {
            core_vr_wait_decrement();
            core_st_do_file(path, core_st_job_save, nullptr, false);
        });

        return 0;
    }

    static int LoadFileSavestate(lua_State* L)
    {
        const std::string path = lua_tostring(L, 1);

        core_vr_wait_increment();
        ThreadPool::submit_task([=] {
            core_vr_wait_decrement();
            core_st_do_file(path, core_st_job_load, nullptr, false);
        });

        return 0;
    }

    static core_st_job lua_to_savestate_job(lua_State* l, const int i)
    {
        const std::string str = lua_tostring(l, i);
        return str == "save" ? core_st_job_save : core_st_job_load;
    }

    static int do_file(lua_State* L)
    {
        const std::filesystem::path path = lua_tostring(L, 1);
        const auto job = lua_to_savestate_job(L, 2);
        const auto callback = lua_tocallback(L, 3);
        const bool ignore_warnings = lua_toboolean(L, 4);

        core_vr_wait_increment();
        ThreadPool::submit_task([=] {
            core_vr_wait_decrement();
            core_st_do_file(path, job, [=](const core_st_callback_info& info, const std::vector<uint8_t>& buf) {
                g_main_window_dispatcher->invoke([=] {
                    if (!get_lua_class(L))
                    {
                        return;
                    }
                    lua_pushcallback(L, callback);
                    lua_pushinteger(L, info.result);
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
        const auto slot = lua_tointeger(L, 1) - 1;
        const auto job = lua_to_savestate_job(L, 2);
        const auto callback = lua_tocallback(L, 3);
        const bool ignore_warnings = lua_toboolean(L, 4);

        core_vr_wait_increment();
        ThreadPool::submit_task([=] {
            core_vr_wait_decrement();
            core_st_do_file(get_st_with_slot_path(slot), job, [=](const core_st_callback_info& info, const std::vector<uint8_t>& buf) {
                g_main_window_dispatcher->invoke([=] {
                    if (!get_lua_class(L))
                    {
                        return;
                    }
                    lua_pushcallback(L, callback);
                    lua_pushinteger(L, info.result);
                    lua_pushlstring(L, (const char*)buf.data(), buf.size());
                    lua_pcall(L, 2, 0, 0);
                });
            },
                            ignore_warnings);
        });
        return 0;
    }

    static int do_memory(lua_State* L)
    {
        size_t buffer_len{};
        const auto buffer_str = lua_tolstring(L, 1, &buffer_len);
        const auto job = lua_to_savestate_job(L, 2);
        const auto callback = lua_tocallback(L, 3);
        const bool ignore_warnings = lua_toboolean(L, 4);

        core_vr_wait_increment();
        ThreadPool::submit_task([=] {
            core_vr_wait_decrement();
            const auto buffer = std::vector<uint8_t>(buffer_str, buffer_str + buffer_len);
            core_st_do_memory(buffer, job, [=](const core_st_callback_info& info, const std::vector<uint8_t>& buf) {
                g_main_window_dispatcher->invoke([=] {
                    if (!get_lua_class(L))
                    {
                        return;
                    }
                    lua_pushcallback(L, callback);
                    lua_pushinteger(L, info.result);
                    lua_pushlstring(L, (const char*)buf.data(), buf.size());
                    lua_pcall(L, 2, 0, 0);
                });
            },
                              ignore_warnings);
        });
        return 0;
    }
} // namespace LuaCore::Savestate
