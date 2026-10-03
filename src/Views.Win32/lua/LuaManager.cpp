/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/ActionManager.hpp>
#include <Common.Views/Config.hpp>
#include <Common.Views/IDialogService.hpp>
#include <lua/LuaCallbacks.hpp>
#include <lua/LuaManager.hpp>
#include <lua/LuaRegistry.hpp>
#include <lua/LuaRenderer.hpp>

CoreButtons g_new_controller_data[4]{};
bool g_overwrite_controller_data[4]{};
size_t g_input_count{};

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

static std::vector<std::shared_ptr<LuaEnvironment>> g_lua_environments{};
std::unordered_map<lua_State *, LuaEnvironment *> g_lua_env_map{};

LuaEnvironment::LuaEnvironment(const std::filesystem::path &path, LuaDestroyFn destroying, LuaPrintFn print)
    : m_l(luaL_newstate()), path(path), destroying(std::move(destroying)), print(std::move(print))
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

static int at_panic(lua_State *L)
{
    const char *raw_msg = lua_tostring(L, -1);
    const std::string_view message = raw_msg ? raw_msg : "";

    DialogService::show_dialog(message, "Lua", CoreMessageTone::Error);

    return 0;
}

static void rebuild_lua_env_map()
{
    g_lua_env_map.clear();
    for (const auto &lua : g_lua_environments)
    {
        g_lua_env_map[lua->l()] = lua.get();
    }
}

const std::vector<std::shared_ptr<LuaEnvironment>> &LuaManager::envs()
{
    return g_lua_environments;
}


LuaEnvironment *LuaManager::get_environment_for_state(lua_State *lua_state)
{
    if (!g_lua_env_map.contains(lua_state))
    {
        return nullptr;
    }
    return g_lua_env_map[lua_state];
}


std::expected<std::shared_ptr<LuaEnvironment>, std::string> LuaEnvironment::create(const std::filesystem::path &path,
    const LuaDestroyFn &destroying_callback, const LuaPrintFn &print_callback)
{
    need(is_on_gui_thread(), "not on GUI thread");

    auto lua = std::make_shared<LuaEnvironment>(path, destroying_callback, print_callback);

    lua->rctx = LuaRenderer::default_rendering_context();

    lua_atpanic(lua->l(), at_panic);
    LuaRegistry::register_functions(lua->l());
    LuaRenderer::create_renderer(&lua->rctx, lua.get());

    return lua;
}

std::expected<void, std::string> LuaEnvironment::start(const bool trusted)
{
    const auto env = shared_from_this();

    if (env->started)
    {
        return std::unexpected("Lua environment already started");
    }

    // We need to put it in the environment list before executing any user code so calls into the Mupen API...
    g_lua_environments.push_back(env);
    rebuild_lua_env_map();

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

    if (!trusted)
    {
        if (luaL_dostring(env->l(), sandbox_lua_code().c_str()))
        {
            has_error = true;
            goto fail;
        }
    }

    // NOTE: We don't want to reach luaL_dofile if the prelude scripts failed, as that would potentially compromise
    // security (if the sandbox script fails for example).
    if (luaL_dofile(env->l(), env->path.string().c_str()))
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

    env->started = true;

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

    // Remove any breakpoints registered by the script.
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

    // This must happen after atstop callbacks while the environment is still valid.
    std::erase_if(g_lua_environments, [&](const auto &active) { return active == env; });
    rebuild_lua_env_map();

    LuaRenderer::destroy_renderer(&env->rctx);

    g_view_logger->info("Lua destroyed");
}
