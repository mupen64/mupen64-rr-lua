/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <ActionManager.h>
#include <lua/LuaManager.h>
#include <lua/modules/Hotkey.h>

namespace LuaCore::Action
{
static ActionManager::t_action_params check_action_params(lua_State *L, int index)
{
    if (lua_gettop(L) < 1 || !lua_istable(L, index))
    {
        luaL_error(L, "Expected a table at argument 1");
        std::unreachable();
    }

    ActionManager::t_action_params params{};

    lua_getfield(L, 1, "path");
    params.path = luaL_checkwstring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "on_press");

    auto on_press = lua_optcallback(L, -1);
    if (on_press)
    {
        params.on_press = [=] {
            if (!LuaManager::get_environment_for_state(L))
            {
                return;
            }

            lua_pushcallback(L, on_press, false);
            lua_pcall(L, 0, 0, 0);
        };
    }

    lua_pop(L, 1);

    lua_getfield(L, 1, "on_release");

    auto on_release = lua_optcallback(L, -1);
    if (on_release)
    {
        params.on_release = [=] {
            if (!LuaManager::get_environment_for_state(L))
            {
                return;
            }

            lua_pushcallback(L, on_release, false);
            lua_pcall(L, 0, 0, 0);
        };
    }

    lua_pop(L, 1);

    lua_getfield(L, 1, "get_display_name");

    auto get_display_name = lua_optcallback(L, -1);
    if (get_display_name)
    {
        params.get_display_name = [=] -> std::wstring {
            if (!LuaManager::get_environment_for_state(L))
            {
                return L"";
            }

            lua_pushcallback(L, get_display_name, false);
            lua_pcall(L, 0, 1, 0);

            const auto display_name = luaL_checkwstring(L, -1);

            return display_name;
        };
    }

    lua_pop(L, 1);

    lua_getfield(L, 1, "get_enabled");

    auto get_enabled = lua_optcallback(L, -1);
    if (get_enabled)
    {
        params.get_enabled = [=] -> bool {
            if (!LuaManager::get_environment_for_state(L))
            {
                return false;
            }

            lua_pushcallback(L, get_enabled, false);
            lua_pcall(L, 0, 1, 0);

            bool enabled = false;
            if (lua_isboolean(L, -1))
            {
                enabled = lua_toboolean(L, -1);
                lua_pop(L, 1);
            }

            return enabled;
        };
    }

    lua_pop(L, 1);

    lua_getfield(L, 1, "get_active");

    auto get_active = lua_optcallback(L, -1);
    if (get_active)
    {
        params.get_active = [=] -> bool {
            if (!LuaManager::get_environment_for_state(L))
            {
                return false;
            }

            lua_pushcallback(L, get_active, false);
            lua_pcall(L, 0, 1, 0);

            bool active = false;
            if (lua_isboolean(L, -1))
            {
                active = lua_toboolean(L, -1);
                lua_pop(L, 1);
            }

            return active;
        };
    }

    lua_pop(L, 1);

    params.on_removed = [=] {
        lua_freecallback(L, on_press);
        lua_freecallback(L, on_release);
        lua_freecallback(L, get_enabled);
        lua_freecallback(L, get_active);
        lua_freecallback(L, get_display_name);
    };

    return params;
}

static int add(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto params = check_action_params(L, 1);

    const auto result = ActionManager::add(params);

    if (result)
    {
        lua->registered_actions.emplace_back(ActionManager::normalize_filter(params.path));
    }

    lua_pushboolean(L, result);
    return 1;
}

static int remove(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto filter = luaL_checkwstring(L, 1);

    const auto removed_actions = ActionManager::remove(filter);

    lua_newtable(L);
    size_t i = 1;
    for (const auto &action : removed_actions)
    {
        std::erase_if(lua->registered_actions,
                      [&](const auto &registered_action) { return registered_action == action; });
        lua_pushstring(L, IOUtils::to_utf8_string(action).c_str());
        lua_seti(L, -2, i++);
    }

    return 1;
}

static int associate_hotkey(lua_State *L)
{
    const auto path = luaL_checkwstring(L, 1);
    const auto hotkey = Hotkey::check_hotkey(L, 2);
    const auto overwrite_existing = (bool)luaL_opt(L, lua_toboolean, 3, false);

    const auto result = ActionManager::associate_hotkey(path, hotkey, overwrite_existing);

    lua_pushboolean(L, result);
    return 1;
}

static int begin_batch_work(lua_State *L)
{
    ActionManager::begin_batch_work();
    return 0;
}

static int end_batch_work(lua_State *L)
{
    ActionManager::end_batch_work();
    return 0;
}

static int notify_display_name_changed(lua_State *L)
{
    const auto filter = luaL_checkwstring(L, 1);
    ActionManager::notify_display_name_changed(filter);
    return 0;
}

static int notify_enabled_changed(lua_State *L)
{
    const auto filter = luaL_checkwstring(L, 1);
    ActionManager::notify_enabled_changed(filter);
    return 0;
}

static int notify_active_changed(lua_State *L)
{
    const auto filter = luaL_checkwstring(L, 1);
    ActionManager::notify_active_changed(filter);
    return 0;
}

static int get_display_name(lua_State *L)
{
    const auto filter = luaL_checkwstring(L, 1);
    const auto ignore_override = (bool)luaL_opt(L, lua_toboolean, 2, false);

    const auto result = ActionManager::get_display_name(filter, ignore_override);

    lua_pushstring(L, IOUtils::to_utf8_string(result).c_str());
    return 1;
}

static int get_enabled(lua_State *L)
{
    const auto path = luaL_checkwstring(L, 1);

    const auto result = ActionManager::get_enabled(path);

    lua_pushboolean(L, result);
    return 1;
}

static int get_active(lua_State *L)
{
    const auto path = luaL_checkwstring(L, 1);

    const auto result = ActionManager::get_active(path);

    lua_pushboolean(L, result);
    return 1;
}

static int get_activatability(lua_State *L)
{
    const auto path = luaL_checkwstring(L, 1);

    const auto result = ActionManager::get_activatability(path);

    lua_pushboolean(L, result);
    return 1;
}

static int get_actions_matching_filter(lua_State *L)
{
    const auto filter = luaL_checkwstring(L, 1);
    const auto actions = ActionManager::get_actions_matching_filter(filter);

    lua_newtable(L);
    size_t i = 1;
    for (const auto &action : actions)
    {
        lua_pushstring(L, IOUtils::to_utf8_string(action).c_str());
        lua_seti(L, -2, i++);
    }

    return 1;
}

static int invoke(lua_State *L)
{
    const auto path = luaL_checkwstring(L, 1);
    const auto up = (bool)luaL_opt(L, lua_toboolean, 2, false);
    const auto release_on_repress = (bool)luaL_opt(L, lua_toboolean, 3, true);

    ActionManager::invoke(path, up, release_on_repress);

    return 0;
}

static int lock_hotkeys(lua_State *L)
{
    const bool lock = luaL_checkboolean(L, 1);

    ActionManager::lock_hotkeys(lock);

    return 0;
}

static int get_hotkeys_locked(lua_State *L)
{
    const bool locked = ActionManager::get_hotkeys_locked();

    lua_pushboolean(L, locked);
    return 1;
}
} // namespace LuaCore::Action
