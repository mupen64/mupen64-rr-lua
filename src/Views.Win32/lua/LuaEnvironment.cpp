/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/ActionManager.hpp>
#include <lua/LuaCallbacks.hpp>
#include <lua/LuaEnvironment.hpp>
#include <lua/LuaHost.hpp>
#include <lua/LuaRegistry.hpp>
#include <lua/LuaRenderer.hpp>

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

LuaEnvironment::LuaEnvironment(const std::filesystem::path &path, LuaDestroyFn destroying, LuaPrintFn print)
    : m_l(luaL_newstate()), m_path(path), destroying(std::move(destroying)), print(std::move(print))
{
    need(is_on_gui_thread(), "LuaEnvironment constructor must be called on the GUI thread");
}

LuaEnvironment::~LuaEnvironment()
{
    if (m_l)
    {
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

    LuaRegistry::register_functions(env->l());

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

    LuaCallbacks::invoke_callbacks_with_key(env.get(), LuaCallbacks::REG_ATSTOP);

    env->destroying(env.get());

    LuaRenderer::pre_destroy_renderer(&env->rctx);

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

    LuaCallbacks::unregister_all(env->l());

    LuaHost::instance().remove_environment(env.get());
    LuaRenderer::destroy_renderer(&env->rctx);

    g_view_logger->info("Lua destroyed");
}
