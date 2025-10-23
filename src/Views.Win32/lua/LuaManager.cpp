/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <ActionManager.h>
#include <Config.h>
#include <DialogService.h>
#include <lua/LuaCallbacks.h>
#include <lua/LuaManager.h>
#include <lua/LuaRegistry.h>
#include <lua/LuaRenderer.h>

core_buttons g_last_controller_data[4]{};
core_buttons g_new_controller_data[4]{};
bool g_overwrite_controller_data[4]{};
size_t g_input_count{};

std::string g_mupen_api_lua_code{};
std::string g_inspect_lua_code{};
std::string g_shims_lua_code{};
std::string g_sandbox_lua_code{};

std::vector<t_lua_environment *> g_lua_environments{};
std::unordered_map<lua_State *, t_lua_environment *> g_lua_env_map{};
std::unordered_map<void *, bool> g_valid_callback_tokens{};

static int at_panic(lua_State *L)
{
    const auto message = IOUtils::to_wide_string(lua_tostring(L, -1));

    g_view_logger->info(L"Lua panic: {}", message);
    DialogService::show_dialog(message.c_str(), L"Lua", fsvc_error);

    return 0;
}

static void rebuild_lua_env_map()
{
    g_lua_env_map.clear();
    for (const auto &lua : g_lua_environments)
    {
        g_lua_env_map[lua->L] = lua;
    }
}

uintptr_t *lua_optcallback(lua_State *L, int i)
{
    if (!lua_isfunction(L, i))
    {
        return nullptr;
    }

    const auto key = new uintptr_t();
    g_valid_callback_tokens[key] = true;

    lua_pushvalue(L, i);
    lua_pushlightuserdata(L, key);
    lua_pushvalue(L, -2);
    lua_settable(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);

    return key;
}

uintptr_t *lua_tocallback(lua_State *L, const int i)
{
    if (!lua_isfunction(L, i))
    {
        luaL_error(L, "Expected a function at argument %d", i);
        return nullptr;
    }

    return lua_optcallback(L, i);
}

void lua_pushcallback(lua_State *L, uintptr_t *token, bool free)
{
    lua_pushlightuserdata(L, token);
    lua_gettable(L, LUA_REGISTRYINDEX);
    if (free)
    {
        lua_freecallback(L, token);
    }
}

void lua_freecallback(lua_State *L, uintptr_t *token)
{
    if (!g_valid_callback_tokens.contains(token))
    {
        return;
    }

    lua_pushlightuserdata(L, token);
    lua_pushnil(L);
    lua_settable(L, LUA_REGISTRYINDEX);

    g_valid_callback_tokens.erase(token);
    delete token;
}

std::wstring luaL_checkwstring(lua_State *L, int i)
{
    if (!lua_isstring(L, i))
    {
        luaL_error(L, "Expected a string at argument %d", i);
    }

    const auto str = lua_tostring(L, i);
    if (str == nullptr)
    {
        luaL_error(L, "Expected a string at argument %d", i);
    }

    return IOUtils::to_wide_string(str);
}

std::wstring luaL_optwstring(lua_State *L, int i, const std::wstring &def)
{
    if (lua_isnoneornil(L, i))
    {
        return def;
    }

    return luaL_checkwstring(L, i);
}

std::wstring lua_pushwstring(lua_State *L, const std::wstring &str)
{
    const auto s = IOUtils::to_utf8_string(str);
    lua_pushstring(L, s.c_str());
    return str;
}

bool luaL_checkboolean(lua_State *L, int i)
{
    if (!lua_isboolean(L, i))
    {
        luaL_error(L, "Expected a boolean at argument %d", i);
    }

    return lua_toboolean(L, i);
}

void LuaManager::init()
{
    g_mupen_api_lua_code = load_resource_as_string(IDR_API_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    g_inspect_lua_code = load_resource_as_string(IDR_INSPECT_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    g_shims_lua_code = load_resource_as_string(IDR_SHIMS_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    g_sandbox_lua_code = load_resource_as_string(IDR_SANDBOX_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
}

t_lua_environment *LuaManager::get_environment_for_state(lua_State *lua_state)
{
    if (!g_lua_env_map.contains(lua_state))
    {
        return nullptr;
    }
    return g_lua_env_map[lua_state];
}

std::expected<t_lua_environment *, std::wstring> LuaManager::create_environment(
    const std::filesystem::path &path, const t_lua_environment::destroying_func &destroying_callback,
    const t_lua_environment::print_func &print_callback)
{
    RT_ASSERT(is_on_gui_thread(), L"not on GUI thread");

    auto lua = new t_lua_environment();

    lua->path = path;
    lua->destroying = destroying_callback;
    lua->print = print_callback;
    lua->rctx = LuaRenderer::default_rendering_context();
    lua->L = luaL_newstate();

    lua_atpanic(lua->L, at_panic);
    LuaRegistry::register_functions(lua->L);
    LuaRenderer::create_renderer(&lua->rctx, lua);

    return lua;
}

std::expected<void, std::wstring> LuaManager::start_environment(t_lua_environment *env, const bool trusted)
{
    if (env->started)
    {
        return std::unexpected(L"Lua environment already started");
    }

    // We need to put it in the environment list before executing any user code so calls into the Mupen API...
    g_lua_environments.push_back(env);
    rebuild_lua_env_map();

    bool has_error = false;

    if (luaL_dostring(env->L, g_mupen_api_lua_code.c_str()))
    {
        has_error = true;
        goto fail;
    }

    LuaRegistry::register_functions(env->L);

    if (luaL_dostring(env->L, g_inspect_lua_code.c_str()))
    {
        has_error = true;
        goto fail;
    }

    if (luaL_dostring(env->L, g_shims_lua_code.c_str()))
    {
        has_error = true;
        goto fail;
    }

    if (!trusted)
    {
        if (luaL_dostring(env->L, g_sandbox_lua_code.c_str()))
        {
            has_error = true;
            goto fail;
        }
    }

    // NOTE: We don't want to reach luaL_dofile if the prelude scripts failed, as that would potentially compromise
    // security (if the sandbox script fails for example).
    if (luaL_dofile(env->L, env->path.string().c_str()))
    {
        has_error = true;
    }

fail:
    if (has_error)
    {
        g_lua_environments.pop_back();
        rebuild_lua_env_map();

        const auto error = IOUtils::to_wide_string(lua_tostring(env->L, -1));
        destroy_environment(env);

        delete env;
        env = nullptr;

        return std::unexpected(error);
    }

    env->started = true;

    return {};
}

void LuaManager::destroy_environment(t_lua_environment *lua)
{
    RT_ASSERT(lua && lua->L, L"LuaManager::destroy_environment: Lua environment is already destroyed");

    LuaCallbacks::invoke_callbacks_with_key(lua, LuaCallbacks::REG_ATSTOP);

    lua->destroying(lua);

    LuaRenderer::pre_destroy_renderer(&lua->rctx);

    ActionManager::begin_batch_work();
    for (const auto &action : lua->registered_actions)
    {
        ActionManager::remove(action);
    }
    ActionManager::end_batch_work();

    // NOTE: We must do this *after* calling atstop, as the lua environment still has to exist for that.
    // After this point, it's game over and no callbacks will be called anymore.
    std::erase_if(g_lua_environments, [=](const t_lua_environment *v) { return v == lua; });
    rebuild_lua_env_map();

    lua_close(lua->L);
    lua->L = nullptr;
    LuaRenderer::destroy_renderer(&lua->rctx);

    g_view_logger->info("Lua destroyed");
}
