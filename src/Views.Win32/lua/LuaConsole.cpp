/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "LuaConsole.h"
#include "Config.h"
#include "DialogService.h"
#include "LuaCallbacks.h"
#include "LuaRegistry.h"
#include <lua/LuaRenderer.h>

const auto INSTANCE_CTX_PROP = L"mup_lua_prop";

core_buttons last_controller_data[4];
core_buttons new_controller_data[4];
bool overwrite_controller_data[4];
size_t g_input_count = 0;

std::vector<t_lua_environment*> g_lua_environments;
std::unordered_map<lua_State*, t_lua_environment*> g_lua_env_map;

std::string mupen_api_lua_code;
std::string inspect_lua_code;
std::string shims_lua_code;

t_lua_environment* get_lua_class(lua_State* lua_state)
{
    if (!g_lua_env_map.contains(lua_state))
    {
        return nullptr;
    }
    return g_lua_env_map[lua_state];
}

int at_panic(lua_State* L)
{
    const auto message = io_service.string_to_wstring(lua_tostring(L, -1));

    g_view_logger->info(L"Lua panic: {}", message);
    DialogService::show_dialog(message.c_str(), L"Lua", fsvc_error);

    return 0;
}

void lua_init()
{
    mupen_api_lua_code = load_resource_as_string(IDR_API_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    inspect_lua_code = load_resource_as_string(IDR_INSPECT_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
    shims_lua_code = load_resource_as_string(IDR_SHIMS_LUA_FILE, MAKEINTRESOURCE(TEXTFILE));
}

static void rebuild_lua_env_map()
{
    g_lua_env_map.clear();
    for (const auto& lua : g_lua_environments)
    {
        g_lua_env_map[lua->L] = lua;
    }
}

void destroy_lua_environment(t_lua_environment* lua)
{
    LuaRenderer::pre_destroy_renderer(&lua->rctx);

    LuaCallbacks::invoke_callbacks_with_key(*lua, LuaCallbacks::REG_ATSTOP);

    // NOTE: We must do this *after* calling atstop, as the lua environment still has to exist for that.
    // After this point, it's game over and no callbacks will be called anymore.
    std::erase_if(g_lua_environments, [=](const t_lua_environment* v) {
        return v == lua;
    });
    lua->wnd_ctx->env = nullptr;
    rebuild_lua_env_map();

    lua_close(lua->L);
    lua->L = nullptr;
    lua->wnd_ctx->destroyed();
    LuaRenderer::destroy_renderer(&lua->rctx);

    g_view_logger->info("Lua destroyed");
}

std::string create_lua_environment(const std::filesystem::path& path, t_lua_wnd_ctx* inst_wnd_ctx, const std::function<void(const std::wstring& path)>& print_callback)
{
    assert(is_on_gui_thread());

    auto lua = new t_lua_environment();

    lua->path = path;
    lua->print = print_callback;
    lua->wnd_ctx = inst_wnd_ctx;
    lua->rctx = LuaRenderer::default_rendering_context();

    lua->L = luaL_newstate();
    lua_atpanic(lua->L, at_panic);
    LuaRegistry::register_functions(lua->L);
    LuaRenderer::create_renderer(&lua->rctx, lua);

    // NOTE: We need to add the lua to the global map already since it may receive callbacks while its executing the global code
    g_lua_environments.push_back(lua);
    inst_wnd_ctx->env = lua;
    rebuild_lua_env_map();

    bool has_error = false;

    {
        ScopeTimer timer("mupenapi.lua injection", g_view_logger.get());
        if (luaL_dostring(lua->L, mupen_api_lua_code.c_str()))
        {
            // Shouldn't happen...
            has_error = true;
        }
    }

    LuaRegistry::register_functions(lua->L);

    {
        ScopeTimer timer("inspect.lua injection", g_view_logger.get());
        if (luaL_dostring(lua->L, inspect_lua_code.c_str()))
        {
            // Shouldn't happen...
            has_error = true;
        }
    }

    {
        ScopeTimer timer("shims.lua injection", g_view_logger.get());
        if (luaL_dostring(lua->L, shims_lua_code.c_str()))
        {
            // Shouldn't happen...
            has_error = true;
        }
    }

    if (luaL_dofile(lua->L, lua->path.string().c_str()))
    {
        has_error = true;
    }

    std::string error_msg;
    if (has_error)
    {
        g_lua_environments.pop_back();
        inst_wnd_ctx->env = nullptr;
        rebuild_lua_env_map();

        error_msg = lua_tostring(lua->L, -1);
        destroy_lua_environment(lua);
        delete lua;
        lua = nullptr;
    }

    return error_msg;
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
