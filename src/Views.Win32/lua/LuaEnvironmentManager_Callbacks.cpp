/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"

#include <lua/LuaEnvironmentManager.hpp>

#define RET_IF_NOT_REGISTERED(key)                                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        if (LuaEnvironmentManager::instance().envs().empty()) return;                                                                \
        if (LuaEnvironmentManager::instance().m_callback_count_map.at(key).load() == 0) return;                                      \
    } while (false)

static int invoke_key_callback(lua_State *l, const LuaKeyEventArgs &args)
{
    lua_newtable(l);
    if (args.keycode.has_value())
    {
        lua_pushstring(l, "keycode");
        lua_pushinteger(l, args.keycode.value());
        lua_settable(l, -3);
    }
    if (args.keycode2.has_value())
    {
        lua_pushstring(l, "keycode2");
        lua_pushinteger(l, args.keycode2.value());
        lua_settable(l, -3);
    }
    if (args.pressed.has_value())
    {
        lua_pushstring(l, "pressed");
        lua_pushboolean(l, args.pressed.value());
        lua_settable(l, -3);
    }
    if (args.text.has_value())
    {
        lua_pushstring(l, "text");
        lua_pushstring(l, args.text.value().c_str());
        lua_settable(l, -3);
    }
    lua_pushstring(l, "ctrl");
    lua_pushboolean(l, args.ctrl);
    lua_settable(l, -3);

    lua_pushstring(l, "alt");
    lua_pushboolean(l, args.alt);
    lua_settable(l, -3);

    lua_pushstring(l, "shift");
    lua_pushboolean(l, args.shift);
    lua_settable(l, -3);

    lua_pushstring(l, "meta");
    lua_pushboolean(l, args.meta);
    lua_settable(l, -3);

    lua_pushstring(l, "repeat");
    lua_pushboolean(l, args.repeat);
    lua_settable(l, -3);
    return lua_pcall(l, 1, 0, 0);
}

static int invoke_mouse_callback(lua_State *l, const LuaMouseEventArgs &args)
{
    lua_newtable(l);

    lua_pushstring(l, "x");
    lua_pushinteger(l, args.x);
    lua_settable(l, -3);

    lua_pushstring(l, "y");
    lua_pushinteger(l, args.y);
    lua_settable(l, -3);

    lua_pushstring(l, "ctrl");
    lua_pushboolean(l, args.ctrl);
    lua_settable(l, -3);

    lua_pushstring(l, "alt");
    lua_pushboolean(l, args.alt);
    lua_settable(l, -3);

    lua_pushstring(l, "shift");
    lua_pushboolean(l, args.shift);
    lua_settable(l, -3);

    lua_pushstring(l, "meta");
    lua_pushboolean(l, args.meta);
    lua_settable(l, -3);

    if (args.x_wheel.has_value())
    {
        lua_pushstring(l, "x_wheel");
        lua_pushinteger(l, args.x_wheel.value());
        lua_settable(l, -3);
    }

    if (args.y_wheel.has_value())
    {
        lua_pushstring(l, "y_wheel");
        lua_pushinteger(l, args.y_wheel.value());
        lua_settable(l, -3);
    }

    if (args.button.has_value())
    {
        lua_pushstring(l, "button");
        lua_pushinteger(l, args.button.value());
        lua_settable(l, -3);
    }

    if (args.pressed.has_value())
    {
        lua_pushstring(l, "pressed");
        lua_pushboolean(l, args.pressed.value());
        lua_settable(l, -3);
    }

    if (args.double_click.has_value())
    {
        lua_pushstring(l, "double_click");
        lua_pushboolean(l, args.double_click.value());
        lua_settable(l, -3);
    }

    if (args.triple_click.has_value())
    {
        lua_pushstring(l, "triple_click");
        lua_pushboolean(l, args.triple_click.value());
        lua_settable(l, -3);
    }

    return lua_pcall(l, 1, 0, 0);
}

static int register_function(lua_State *L, LuaEnvironmentManager::callback_key key)
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

static void unregister_function(lua_State *L, LuaEnvironmentManager::callback_key key)
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

LuaEnvironmentManager::LuaEnvironmentManager()
{
    for (uint8_t i = LuaEnvironmentManager::callback_key::REG_LUACLASS; i < LuaEnvironmentManager::callback_key::_COUNT; ++i)
        m_callback_count_map.emplace(i, 0);
}

void LuaEnvironmentManager::call_window_message(void *wnd, unsigned int msg, std::uintptr_t w, std::intptr_t l)
{
    RET_IF_NOT_REGISTERED(REG_WINDOWMESSAGE);

    g_main_ctx.dispatcher->invoke([=] {
        LuaEnvironmentManager::instance().call_by_key(REG_WINDOWMESSAGE, [=](lua_State *state) {
            lua_pushinteger(state, (lua_Integer)wnd);
            lua_pushinteger(state, msg);
            lua_pushinteger(state, w);
            lua_pushinteger(state, l);
            return lua_pcall(state, 4, 0, 0);
        });
    });
}

void LuaEnvironmentManager::call_vi()
{
    RET_IF_NOT_REGISTERED(REG_ATVI);
    g_main_ctx.dispatcher->invoke([] { LuaEnvironmentManager::instance().call_by_key(REG_ATVI); });
}

void LuaEnvironmentManager::call_input(CoreButtons *input, int index)
{
    RET_IF_NOT_REGISTERED(REG_ATINPUT);

    g_main_ctx.dispatcher->invoke([=] {
        LuaEnvironmentManager::instance().call_by_key(REG_ATINPUT, [index](lua_State *l) {
            lua_pushinteger(l, index);
            return lua_pcall(l, 1, 0, 0);
        });
        LuaEnvironmentManager::instance().input_count++;
    });

    if (LuaEnvironmentManager::instance().overwrite_controller_data[index])
    {
        *input = LuaEnvironmentManager::instance().new_controller_data[index];
        g_main_ctx.last_controller_data[index] = *input;
        LuaEnvironmentManager::instance().overwrite_controller_data[index] = false;
    }
}

void LuaEnvironmentManager::call_interval()
{
    RET_IF_NOT_REGISTERED(REG_ATINTERVAL);
    g_main_ctx.dispatcher->invoke([] { LuaEnvironmentManager::instance().call_by_key(REG_ATINTERVAL); });
}

void LuaEnvironmentManager::call_play_movie()
{
    RET_IF_NOT_REGISTERED(REG_ATPLAYMOVIE);
    g_main_ctx.dispatcher->invoke([] { LuaEnvironmentManager::instance().call_by_key(REG_ATPLAYMOVIE); });
}

void LuaEnvironmentManager::call_stop_movie()
{
    RET_IF_NOT_REGISTERED(REG_ATSTOPMOVIE);
    g_main_ctx.dispatcher->invoke([] { LuaEnvironmentManager::instance().call_by_key(REG_ATSTOPMOVIE); });
}

void LuaEnvironmentManager::call_load_state()
{
    RET_IF_NOT_REGISTERED(REG_ATLOADSTATE);
    g_main_ctx.dispatcher->invoke([] { LuaEnvironmentManager::instance().call_by_key(REG_ATLOADSTATE); });
}

void LuaEnvironmentManager::call_save_state()
{
    RET_IF_NOT_REGISTERED(REG_ATSAVESTATE);
    g_main_ctx.dispatcher->invoke([] { LuaEnvironmentManager::instance().call_by_key(REG_ATSAVESTATE); });
}

void LuaEnvironmentManager::call_reset()
{
    RET_IF_NOT_REGISTERED(REG_ATRESET);
    g_main_ctx.dispatcher->invoke([] { LuaEnvironmentManager::instance().call_by_key(REG_ATRESET); });
}

void LuaEnvironmentManager::call_seek_completed()
{
    RET_IF_NOT_REGISTERED(REG_ATSEEKCOMPLETED);
    g_main_ctx.dispatcher->invoke([] { LuaEnvironmentManager::instance().call_by_key(REG_ATSEEKCOMPLETED); });
}

void LuaEnvironmentManager::call_warp_modify_status_changed(const int32_t status)
{
    RET_IF_NOT_REGISTERED(REG_ATWARPMODIFYSTATUSCHANGED);
    g_main_ctx.dispatcher->invoke([] {
        LuaEnvironmentManager::instance().call_by_key(REG_ATWARPMODIFYSTATUSCHANGED, [](lua_State *l) {
            lua_pushinteger(l, g_main_ctx.CoreCtx->vcr_get_warp_modify_status());
            return lua_pcall(l, 1, 0, 0);
        });
    });
}

void LuaEnvironmentManager::call_atkey(const LuaKeyEventArgs &args)
{
    RET_IF_NOT_REGISTERED(REG_ATKEY);
    g_main_ctx.dispatcher->invoke([args] {
        LuaEnvironmentManager::instance().call_by_key(REG_ATKEY, [&args](lua_State *l) { return invoke_key_callback(l, args); });
    });
}

void LuaEnvironmentManager::call_atmouse(const LuaMouseEventArgs &args)
{
    RET_IF_NOT_REGISTERED(REG_ATMOUSE);
    g_main_ctx.dispatcher->invoke([args] {
        LuaEnvironmentManager::instance().call_by_key(REG_ATMOUSE, [&args](lua_State *l) { return invoke_mouse_callback(l, args); });
    });
}

bool invoke_callbacks_with_key_impl(
    const LuaEnvironment *lua, const std::function<int(lua_State *)> &function, LuaEnvironmentManager::callback_key key)
{
    need(is_on_gui_thread(), "not on GUI thread");

    lua_State *L = lua->l();

    lua_rawgeti(L, LUA_REGISTRYINDEX, key);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        return true;
    }

    const lua_Integer n = luaL_len(L, -1);

    for (lua_Integer i = 0; i < n; i++)
    {
        lua_pushinteger(L, 1 + i);
        lua_gettable(L, -2);
        if (function(L))
        {
            const char *str = lua_tostring(L, -1);
            const std::string message = str ? str : "Lua callback failed with a non-string error object";
            lua->print(lua, message + "\r\n");
            g_view_logger->info("Lua error: {}", message);
            return false;
        }
    }
    lua_pop(L, 1);
    return true;
}

bool LuaEnvironmentManager::call_by_key(
    const LuaEnvironment *lua, const callback_key key, const std::function<int(lua_State *)> &function)
{
    return invoke_callbacks_with_key_impl(lua, function, key);
}

void LuaEnvironmentManager::call_by_key(callback_key key, const std::function<int(lua_State *)> &function)
{
    // OPTIMIZATION: Store destruction-queued scripts in queue and destroy them after iteration to avoid having to clone
    // the queue OPTIMIZATION: Make the destruction queue static to avoid allocating it every entry
    static std::queue<std::shared_ptr<LuaEnvironment>> destruction_queue;

    need(destruction_queue.empty(), "destruction_queue must be empty");

    for (const auto &lua : LuaEnvironmentManager::instance().envs())
    {
        if (!invoke_callbacks_with_key_impl(lua.get(), function, key))
        {
            destruction_queue.push(lua);
        }
    }

    while (!destruction_queue.empty())
    {
        destruction_queue.front()->stop();
        destruction_queue.pop();
    }
}

void LuaEnvironment::register_or_unregister_function(const uint8_t callback_key)
{
    lua_State *state = l();
    const auto key = static_cast<LuaEnvironmentManager::callback_key>(callback_key);

    if (lua_toboolean(state, 2))
    {
        lua_pop(state, 1);
        unregister_function(state, key);
        LuaEnvironmentManager::instance().m_callback_count_map[key]--;
    }
    else
    {
        if (lua_gettop(state) == 2) lua_pop(state, 1);
        register_function(state, key);
        LuaEnvironmentManager::instance().m_callback_count_map[key]++;
    }
}
