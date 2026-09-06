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

std::string g_mupen_api_lua_code{};
std::string g_inspect_lua_code{};
std::string g_shims_lua_code{};
std::string g_sandbox_lua_code{};

std::vector<LuaEnvironment *> g_lua_environments{};
std::unordered_map<lua_State *, LuaEnvironment *> g_lua_env_map{};

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
        g_lua_env_map[lua->L] = lua;
    }
}

void LuaManager::init()
{
    g_mupen_api_lua_code = load_resource_as_string(IDR_API_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    g_inspect_lua_code = load_resource_as_string(IDR_INSPECT_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    g_shims_lua_code = load_resource_as_string(IDR_SHIMS_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    g_sandbox_lua_code = load_resource_as_string(IDR_SANDBOX_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
}

LuaEnvironment *LuaManager::get_environment_for_state(lua_State *lua_state)
{
    if (!g_lua_env_map.contains(lua_state))
    {
        return nullptr;
    }
    return g_lua_env_map[lua_state];
}

std::expected<LuaEnvironment *, std::string> LuaManager::create_environment(const std::filesystem::path &path,
    const LuaEnvironment::destroying_func &destroying_callback, const LuaEnvironment::print_func &print_callback)
{
    need(is_on_gui_thread(), "not on GUI thread");

    auto lua = new LuaEnvironment();

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

std::expected<void, std::string> LuaManager::start_environment(LuaEnvironment *env, const bool trusted)
{
    if (env->started)
    {
        return std::unexpected("Lua environment already started");
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

        const auto error = lua_tostring(env->L, -1);
        destroy_environment(env);

        delete env;
        env = nullptr;

        return std::unexpected(error);
    }

    env->started = true;

    return {};
}

void LuaManager::destroy_environment(LuaEnvironment *lua)
{
    need(lua && lua->L, "LuaManager::destroy_environment: Lua environment is already destroyed");

    LuaCallbacks::invoke_callbacks_with_key(lua, LuaCallbacks::REG_ATSTOP);

    lua->destroying(lua);

    LuaRenderer::pre_destroy_renderer(&lua->rctx);

    ActionManager::begin_batch_work();
    for (const auto &action : lua->registered_actions)
    {
        ActionManager::remove(action);
    }
    ActionManager::end_batch_work();

    // Remove any breakpoints registered by the script.
    for (const auto &pair : lua->active_breakpoints)
    {
        g_main_ctx.CoreCtx->dbg_remove_breakpoint(pair.first);
        lua_freecallback(lua->L, pair.second);
    }

    for (auto callback : lua->step_callbacks)
    {
        lua_freecallback(lua->L, callback);
    }

    LuaCallbacks::unregister_all(lua->L);

    // NOTE: We must do this *after* calling atstop, as the lua environment still has to exist for that.
    // After this point, it's game over and no callbacks will be called anymore.
    std::erase_if(g_lua_environments, [=](const LuaEnvironment *v) { return v == lua; });
    rebuild_lua_env_map();

    lua_close(lua->L);
    lua->L = nullptr;
    LuaRenderer::destroy_renderer(&lua->rctx);

    g_view_logger->info("Lua destroyed");
}
