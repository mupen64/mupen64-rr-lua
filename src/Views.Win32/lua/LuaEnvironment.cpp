/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/ActionManager.hpp>
#include <lua/LuaHost.hpp>
#include <lua/LuaRendererManager.hpp>

static const std::string &mupen_api_lua_code()
{
    static const std::string code = load_resource_as_string(IDR_API_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    return code;
}

static const std::string &inspect_lua_code()
{
    static const std::string code = load_resource_as_string(IDR_INSPECT_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    return code;
}

static const std::string &sandbox_lua_code()
{
    static const std::string code = load_resource_as_string(IDR_SANDBOX_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    return code;
}

LuaEnvironment::LuaEnvironment(const std::filesystem::path &path, LuaStoppingFn stopping, LuaPrintFn print)
    : m_l(luaL_newstate()), m_path(path), stopping(std::move(stopping)), print(std::move(print))
{
    need(is_on_gui_thread(), "LuaEnvironment constructor must be called on the GUI thread");
}

std::shared_ptr<LuaEnvironment> LuaEnvironment::create(
    const std::filesystem::path &path, LuaStoppingFn stopping, LuaPrintFn print)
{
    return std::shared_ptr<LuaEnvironment>(new LuaEnvironment(path, std::move(stopping), std::move(print)));
}

LuaEnvironment::~LuaEnvironment()
{
    if (m_l)
    {
        lua_freecallbacks(m_l);
        lua_close(m_l);
        m_l = nullptr;
    }
}

std::expected<void, std::string> LuaEnvironment::start(const bool trusted)
{
    const auto env = shared_from_this();

    if (env->m_started)
    {
        return std::unexpected("Lua environment already started");
    }

    // Register before executing user code so API calls can find this environment.
    LuaHost::instance().add_environment(env);

    bool has_error = false;

    if (luaL_dostring(env->l(), mupen_api_lua_code().c_str()))
    {
        has_error = true;
        goto fail;
    }

    LuaHost::instance().register_functions(env->l());

    if (luaL_dostring(env->l(), inspect_lua_code().c_str()))
    {
        has_error = true;
        goto fail;
    }

    lua_getglobal(env->l(), "__mupen_apply_shims");
    if (!lua_isfunction(env->l(), -1) || lua_pcall(env->l(), 0, 0, 0))
    {
        has_error = true;
        goto fail;
    }
    lua_pushnil(env->l());
    lua_setglobal(env->l(), "__mupen_apply_shims");

    if (!trusted && luaL_dostring(env->l(), sandbox_lua_code().c_str()))
    {
        has_error = true;
        goto fail;
    }

    // Do not run the user script if a prelude failed, as that could compromise the sandbox.
    if (luaL_dofile(env->l(), env->m_path.string().c_str()))
    {
        has_error = true;
    }

fail:
    if (has_error)
    {
        const std::string error = lua_tostring(env->l(), -1);
        env->stop();
        return std::unexpected(error);
    }

    env->m_started = true;
    return {};
}

void LuaEnvironment::stop()
{
    const auto env = shared_from_this();
    need(env->l(), "LuaEnvironment::stop: Lua environment is already stopped");

    LuaHost::instance().call_by_key(env.get(), LuaHost::REG_ATSTOP);

    env->stopping(env.get());

    env->renderer.pre_shutdown();

    ActionManager::begin_batch_work();
    for (const auto &action : env->registered_actions)
    {
        ActionManager::remove(action);
    }
    ActionManager::end_batch_work();

    for (const auto &pair : env->active_breakpoints)
    {
        g_main_ctx.CoreCtx->dbg_remove_breakpoint(pair.first);
        lua_freecallback(env->l(), pair.second);
    }

    for (auto callback : env->step_callbacks)
    {
        lua_freecallback(env->l(), callback);
    }

    for (auto &[key, count] : LuaHost::instance().m_callback_count_map)
    {
        lua_rawgeti(env->l(), LUA_REGISTRYINDEX, key);
        if (lua_isnil(env->l(), -1))
        {
            lua_pop(env->l(), 1);
            continue;
        }

        const int n = luaL_len(env->l(), -1);
        g_view_logger->trace(L"Unsubscribing {} functions of key {}...", n, static_cast<int>(key));

        LuaHost::instance().m_callback_count_map[key] -= n;

        lua_newtable(env->l());
        lua_rawseti(env->l(), LUA_REGISTRYINDEX, key);

        lua_pop(env->l(), 0);
    }

    LuaHost::instance().remove_environment(env.get());
    env->renderer.shutdown();

    g_view_logger->info("Lua destroyed");
}
