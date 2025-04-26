/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
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

struct t_atwindowmessage_context {
    HWND wnd;
    UINT msg;
    WPARAM w_param;
    LPARAM l_param;
};

static t_atwindowmessage_context atwindowmessage_ctx{};
static int current_input_n = 0;

const std::unordered_map<LuaCallbacks::callback_key, std::function<int(lua_State*)>> CALLBACK_FUNC_MAP = {
{LuaCallbacks::REG_ATINPUT, [](auto l) -> int {
     lua_pushinteger(l, current_input_n);
     return lua_pcall(l, 1, 0, 0);
 }},
{LuaCallbacks::REG_WINDOWMESSAGE, [](auto l) -> int {
     lua_pushinteger(l, (unsigned)atwindowmessage_ctx.wnd);
     lua_pushinteger(l, atwindowmessage_ctx.msg);
     lua_pushinteger(l, atwindowmessage_ctx.w_param);
     lua_pushinteger(l, atwindowmessage_ctx.l_param);
     return lua_pcall(l, 4, 0, 0);
 }},
{LuaCallbacks::REG_ATWARPMODIFYSTATUSCHANGED, [](auto l) -> int {
     lua_pushinteger(l, core_vcr_get_warp_modify_status());
     return lua_pcall(l, 1, 0, 0);
 }},
};

static std::function<int(lua_State*)> get_function_for_callback(const LuaCallbacks::callback_key key)
{
    if (CALLBACK_FUNC_MAP.contains(key))
    {
        return CALLBACK_FUNC_MAP.at(key);
    }
    return pcall_no_params;
}

core_buttons LuaCallbacks::get_last_controller_data(int index)
{
    return last_controller_data[index];
}

void LuaCallbacks::call_window_message(void* wnd, unsigned int msg, unsigned int w, long l)
{
    RET_IF_EMPTY;

    atwindowmessage_ctx = {
    .wnd = (HWND)wnd,
    .msg = msg,
    .w_param = w,
    .l_param = l};

    g_main_window_dispatcher->invoke([] {
        invoke_callbacks_with_key_on_all_instances(REG_WINDOWMESSAGE);
    });
}

void LuaCallbacks::call_vi()
{
    RET_IF_EMPTY;
    g_main_window_dispatcher->invoke([] {
        invoke_callbacks_with_key_on_all_instances(REG_ATVI);
    });
}

void LuaCallbacks::call_input(core_buttons* input, int index)
{
    // NOTE: Special callback, we store the input data for all scripts to access via joypad.get(n)
    // If they request a change via joypad.set(n, input), we change the input
    last_controller_data[index] = *input;

    RET_IF_EMPTY;

    g_main_window_dispatcher->invoke([=] {
        current_input_n = index;
        invoke_callbacks_with_key_on_all_instances(REG_ATINPUT);
        g_input_count++;
    });

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
    g_main_window_dispatcher->invoke([] {
        invoke_callbacks_with_key_on_all_instances(REG_ATINTERVAL);
    });
}

void LuaCallbacks::call_play_movie()
{
    RET_IF_EMPTY;
    g_main_window_dispatcher->invoke([] {
        invoke_callbacks_with_key_on_all_instances(REG_ATPLAYMOVIE);
    });
}

void LuaCallbacks::call_stop_movie()
{
    RET_IF_EMPTY;
    g_main_window_dispatcher->invoke([] {
        invoke_callbacks_with_key_on_all_instances(REG_ATSTOPMOVIE);
    });
}

void LuaCallbacks::call_load_state()
{
    RET_IF_EMPTY;
    g_main_window_dispatcher->invoke([] {
        invoke_callbacks_with_key_on_all_instances(REG_ATLOADSTATE);
    });
}

void LuaCallbacks::call_save_state()
{
    RET_IF_EMPTY;
    g_main_window_dispatcher->invoke([] {
        invoke_callbacks_with_key_on_all_instances(REG_ATSAVESTATE);
    });
}

void LuaCallbacks::call_reset()
{
    RET_IF_EMPTY;
    g_main_window_dispatcher->invoke([] {
        invoke_callbacks_with_key_on_all_instances(REG_ATRESET);
    });
}

void LuaCallbacks::call_seek_completed()
{
    RET_IF_EMPTY;
    g_main_window_dispatcher->invoke([] {
        invoke_callbacks_with_key_on_all_instances(REG_ATSEEKCOMPLETED);
    });
}

void LuaCallbacks::call_warp_modify_status_changed(const int32_t status)
{
    RET_IF_EMPTY;
    g_main_window_dispatcher->invoke([=] {
        invoke_callbacks_with_key_on_all_instances(REG_ATWARPMODIFYSTATUSCHANGED);
    });
}

bool invoke_callbacks_with_key_impl(const t_lua_environment& lua, const std::function<int(lua_State*)>& function, LuaCallbacks::callback_key key)
{
    assert(is_on_gui_thread());

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

bool LuaCallbacks::invoke_callbacks_with_key(const t_lua_environment& lua, callback_key key)
{
    return invoke_callbacks_with_key_impl(lua, get_function_for_callback(key), key);
}

void LuaCallbacks::invoke_callbacks_with_key_on_all_instances(callback_key key)
{
    // OPTIMIZATION: Store destruction-queued scripts in queue and destroy them after iteration to avoid having to clone the queue
    // OPTIMIZATION: Make the destruction queue static to avoid allocating it every entry
    static std::queue<t_lua_environment*> destruction_queue;

    assert(destruction_queue.empty());

    const auto function = get_function_for_callback(key);

    for (const auto& lua : g_lua_environments)
    {
        if (!invoke_callbacks_with_key_impl(*lua, function, key))
        {
            destruction_queue.push(lua);
        }
    }

    while (!destruction_queue.empty())
    {
        destroy_lua_environment(destruction_queue.front());
        destruction_queue.pop();
    }
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
        lua_newtable(L);
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
