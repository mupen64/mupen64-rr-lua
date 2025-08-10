/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <lua/LuaManager.h>
#include <Config.h>
#include <DialogService.h>
#include <lua/LuaCallbacks.h>
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

std::vector<t_lua_environment*> g_lua_environments{};
std::unordered_map<lua_State*, t_lua_environment*> g_lua_env_map{};

static int at_panic(lua_State* L)
{
    const auto message = io_service.string_to_wstring(lua_tostring(L, -1));

    g_view_logger->info(L"Lua panic: {}", message);
    DialogService::show_dialog(message.c_str(), L"Lua", fsvc_error);

    return 0;
}

static void rebuild_lua_env_map()
{
    g_lua_env_map.clear();
    for (const auto& lua : g_lua_environments)
    {
        g_lua_env_map[lua->L] = lua;
    }
}

void* lua_tocallback(lua_State* L, const int i)
{
    void* key = calloc(1, sizeof(void*));
    lua_pushvalue(L, i);
    lua_pushlightuserdata(L, key);
    lua_pushvalue(L, -2);
    lua_settable(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);
    return key;
}

void lua_pushcallback(lua_State* L, void* key)
{
    lua_pushlightuserdata(L, key);
    lua_gettable(L, LUA_REGISTRYINDEX);
    free(key);
    key = nullptr;
}

void LuaManager::init()
{
    g_mupen_api_lua_code = load_resource_as_string(IDR_API_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    g_inspect_lua_code = load_resource_as_string(IDR_INSPECT_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    g_shims_lua_code = load_resource_as_string(IDR_SHIMS_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    g_sandbox_lua_code = load_resource_as_string(IDR_SANDBOX_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
}

t_lua_environment* LuaManager::get_environment_for_state(lua_State* lua_state)
{
    if (!g_lua_env_map.contains(lua_state))
    {
        return nullptr;
    }
    return g_lua_env_map[lua_state];
}

std::expected<t_lua_environment*, std::wstring> LuaManager::create_environment(const std::filesystem::path& path, const bool trusted, const t_lua_environment::destroying_func& destroying_callback, const t_lua_environment::print_func& print_callback)
{
    assert(is_on_gui_thread());

    auto lua = new t_lua_environment();

    lua->path = path;
    lua->destroying = destroying_callback;
    lua->print = print_callback;
    lua->rctx = LuaRenderer::default_rendering_context();

    lua->L = luaL_newstate();
    lua_atpanic(lua->L, at_panic);
    LuaRegistry::register_functions(lua->L);
    LuaRenderer::create_renderer(&lua->rctx, lua);

    // NOTE: We need to add the lua to the global map already since it may receive callbacks while its executing the global code
    g_lua_environments.push_back(lua);
    rebuild_lua_env_map();

    bool has_error = false;

    if (luaL_dostring(lua->L, g_mupen_api_lua_code.c_str()))
    {
        has_error = true;
        goto fail;
    }

    LuaRegistry::register_functions(lua->L);

    if (luaL_dostring(lua->L, g_inspect_lua_code.c_str()))
    {
        has_error = true;
        goto fail;
    }

    if (luaL_dostring(lua->L, g_shims_lua_code.c_str()))
    {
        has_error = true;
        goto fail;
    }

    if (!trusted)
    {
        if (luaL_dostring(lua->L, g_sandbox_lua_code.c_str()))
        {
            has_error = true;
            goto fail;
        }
    }

    // NOTE: We don't want to reach luaL_dofile if the prelude scripts failed, as that would potentially compromise security (if the sandbox script fails for example).
    if (luaL_dofile(lua->L, lua->path.string().c_str()))
    {
        has_error = true;
    }

fail:
    if (has_error)
    {
        g_lua_environments.pop_back();
        rebuild_lua_env_map();

        const auto error = io_service.string_to_wstring(lua_tostring(lua->L, -1));
        destroy_environment(lua);

        delete lua;
        lua = nullptr;

        return std::unexpected(error);
    }

    return lua;
}

void LuaManager::destroy_environment(t_lua_environment* lua)
{
    runtime_assert(lua && lua->L, L"LuaManager::destroy_environment: Lua environment is already destroyed");

    lua->destroying(lua);
    
    LuaRenderer::pre_destroy_renderer(&lua->rctx);

    LuaCallbacks::invoke_callbacks_with_key(lua, LuaCallbacks::REG_ATSTOP);

    // NOTE: We must do this *after* calling atstop, as the lua environment still has to exist for that.
    // After this point, it's game over and no callbacks will be called anymore.
    std::erase_if(g_lua_environments, [=](const t_lua_environment* v) {
        return v == lua;
    });
    rebuild_lua_env_map();

    lua_close(lua->L);
    lua->L = nullptr;
    LuaRenderer::destroy_renderer(&lua->rctx);
    
    g_view_logger->info("Lua destroyed");
}
