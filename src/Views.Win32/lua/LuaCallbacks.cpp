/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <AsyncExecutor.h>
#include <lua/LuaCallbacks.h>
#include <lua/LuaConsole.h>
#include <Main.h>

// OPTIMIZATION: If no lua scripts are running, skip the deeper lua path
// This is an unsynchronized access to the map from the emu thread!
#define RET_IF_EMPTY                    \
    {                                   \
        if (g_lua_environments.empty()) \
            return;                     \
    }

typedef struct {
    HWND wnd;
    UINT msg;
    WPARAM w_param;
    LPARAM l_param;
} t_window_procedure_params;

t_window_procedure_params window_proc_params{};

int current_input_n = 0;

int AtInput(lua_State* L)
{
    lua_pushinteger(L, current_input_n);
    return lua_pcall(L, 1, 0, 0);
}

int AtWindowMessage(lua_State* L)
{
    lua_pushinteger(L, (unsigned)window_proc_params.wnd);
    lua_pushinteger(L, window_proc_params.msg);
    lua_pushinteger(L, window_proc_params.w_param);
    lua_pushinteger(L, window_proc_params.l_param);

    return lua_pcall(L, 4, 0, 0);
}

core_buttons LuaCallbacks::get_last_controller_data(int index)
{
    return last_controller_data[index];
}

void LuaCallbacks::call_window_message(void* wnd, unsigned int msg, unsigned int w, long l)
{
    RET_IF_EMPTY;
    // Invoking dispatcher here isn't allowed, as it would lead to infinite recursion
    window_proc_params = {
    .wnd = (HWND)wnd,
    .msg = msg,
    .w_param = w,
    .l_param = l};
    invoke_callbacks_with_key_on_all_instances(AtWindowMessage, REG_WINDOWMESSAGE);
}

void LuaCallbacks::call_vi()
{
    RET_IF_EMPTY;
    invoke_callbacks_with_key_on_all_instances(pcall_no_params, REG_ATVI);
}

void LuaCallbacks::call_input(core_buttons* input, int index)
{
    // NOTE: Special callback, we store the input data for all scripts to access via joypad.get(n)
    // If they request a change via joypad.set(n, input), we change the input
    last_controller_data[index] = *input;

    RET_IF_EMPTY;

    current_input_n = index;
    invoke_callbacks_with_key_on_all_instances(AtInput, REG_ATINPUT).wait();
    g_input_count++;

    if (overwrite_controller_data[index])
    {
        *input = new_controller_data[index];
        last_controller_data[index] = *input;
        overwrite_controller_data[index] = false;
    }
}

void LuaCallbacks::call_interval()
{
    RET_IF_EMPTY;
    invoke_callbacks_with_key_on_all_instances(pcall_no_params, REG_ATINTERVAL);
}

void LuaCallbacks::call_play_movie()
{
    RET_IF_EMPTY;
    invoke_callbacks_with_key_on_all_instances(pcall_no_params, REG_ATPLAYMOVIE);
}

void LuaCallbacks::call_stop_movie()
{
    RET_IF_EMPTY;
    invoke_callbacks_with_key_on_all_instances(pcall_no_params, REG_ATSTOPMOVIE);
}

void LuaCallbacks::call_load_state()
{
    RET_IF_EMPTY;
    invoke_callbacks_with_key_on_all_instances(pcall_no_params, REG_ATLOADSTATE);
}

void LuaCallbacks::call_save_state()
{
    RET_IF_EMPTY;
    invoke_callbacks_with_key_on_all_instances(pcall_no_params, REG_ATSAVESTATE);
}

void LuaCallbacks::call_reset()
{
    RET_IF_EMPTY;
    invoke_callbacks_with_key_on_all_instances(pcall_no_params, REG_ATRESET);
}

void LuaCallbacks::call_seek_completed()
{
    RET_IF_EMPTY;
    invoke_callbacks_with_key_on_all_instances(pcall_no_params, REG_ATSEEKCOMPLETED);
}

void LuaCallbacks::call_warp_modify_status_changed(const int32_t status)
{
    RET_IF_EMPTY;
    invoke_callbacks_with_key_on_all_instances(
    [=](lua_State* L) {
        lua_pushinteger(L, status);
        return lua_pcall(L, 1, 0, 0);
    },
    REG_ATWARPMODIFYSTATUSCHANGED);
}

bool LuaCallbacks::invoke_callbacks_with_key(const LuaEnvironment& lua, const std::function<int(lua_State*)>& function, callback_key key)
{
    std::scoped_lock lock(lua_mutex);

    lua_rawgeti(lua.L, LUA_REGISTRYINDEX, key);
    if (lua_isnil(lua.L, -1))
    {
        lua_pop(lua.L, 1);
        return true;
    }
    int n = luaL_len(lua.L, -1);
    for (LUA_INTEGER i = 0; i < n; i++)
    {
        lua_pushinteger(lua.L, 1 + i);
        lua_gettable(lua.L, -2);
        if (function(lua.L))
        {
            const char* str = lua_tostring(lua.L, -1);
            print_con(lua.hwnd, string_to_wstring(str) + L"\r\n");
            g_view_logger->info("Lua error: {}", str);
            return false;
        }
    }
    lua_pop(lua.L, 1);
    return true;
}

std::future<void> LuaCallbacks::invoke_callbacks_with_key_on_all_instances(const std::function<int(lua_State*)>& function, callback_key key)
{
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    AsyncExecutor::invoke_async([=] {
        std::scoped_lock lock(lua_mutex);

        // OPTIMIZATION: Store destruction-queued scripts in queue and destroy them after iteration to avoid having to clone the queue
        // OPTIMIZATION: Make the destruction queue static to avoid allocating it every entry
        static std::queue<LuaEnvironment*> destruction_queue;

        assert(destruction_queue.empty());

        for (const auto& lua : g_lua_environments)
        {
            if (!invoke_callbacks_with_key(*lua, function, key))
            {
                destruction_queue.push(lua);
            }
        }

        while (!destruction_queue.empty())
        {
            destroy_lua_environment(destruction_queue.front());
            destruction_queue.pop();
        }

        promise->set_value();
    });

    return future;
}

static int register_function(lua_State* L, LuaCallbacks::callback_key key)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, key);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_rawseti(L, LUA_REGISTRYINDEX, key);
        lua_rawgeti(L, LUA_REGISTRYINDEX, key);
    }
    int i = luaL_len(L, -1) + 1;
    lua_pushinteger(L, i);
    lua_pushvalue(L, -3); //
    lua_settable(L, -3);
    lua_pop(L, 1);
    return i;
}

static void unregister_function(lua_State* L, LuaCallbacks::callback_key key)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, key);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L); // �Ƃ肠����
    }
    int n = luaL_len(L, -1);
    for (LUA_INTEGER i = 0; i < n; i++)
    {
        lua_pushinteger(L, 1 + i);
        lua_gettable(L, -2);
        if (lua_rawequal(L, -1, -3))
        {
            lua_pop(L, 1);
            lua_getglobal(L, "table");
            lua_getfield(L, -1, "remove");
            lua_pushvalue(L, -3);
            lua_pushinteger(L, 1 + i);
            lua_call(L, 2, 0);
            lua_pop(L, 2);
            return;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_pushfstring(L, "unregister_function(%s): not found function", key);
    lua_error(L);
}

void LuaCallbacks::register_or_unregister_function(lua_State* l, const callback_key key)
{
    if (lua_toboolean(l, 2))
    {
        lua_pop(l, 1);
        unregister_function(l, key);
    }
    else
    {
        if (lua_gettop(l) == 2)
            lua_pop(l, 1);
        register_function(l, key);
    }
}
