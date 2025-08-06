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
    static bool check_action_params(lua_State* L, ActionManager::t_action_params& params)
    {
        if (lua_gettop(L) < 1 || !lua_istable(L, 1))
        {
            return false;
        }

        lua_getfield(L, 1, "path");
        params.path = lua_getwstring(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, 1, "down_callback");

        auto down_callback = lua_tocallback(L, -1);
        params.down_callback = [=] {
            if (!LuaManager::get_environment_for_state(L))
            {
                return;
            }

            lua_pushcallback(L, down_callback, false);
            lua_pcall(L, 0, 0, 0);
        };

        lua_pop(L, 1);

        lua_getfield(L, 1, "up_callback");

        auto up_callback = lua_optcallback(L, -1);
        if (up_callback)
        {
            params.up_callback = [=] {
                if (!LuaManager::get_environment_for_state(L))
                {
                    return;
                }

                lua_pushcallback(L, up_callback, false);
                lua_pcall(L, 0, 0, 0);
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

        lua_getfield(L, 1, "get_real_name");

        auto get_real_name = lua_optcallback(L, -1);
        if (get_real_name)
        {
            params.get_real_name = [=] -> std::wstring {
                if (!LuaManager::get_environment_for_state(L))
                {
                    return L"";
                }

                lua_pushcallback(L, get_real_name, false);
                lua_pcall(L, 0, 1, 0);

                std::wstring real_name;
                if (lua_isstring(L, -1))
                {
                    real_name = lua_towstring(L, -1);
                    lua_pop(L, 1);
                }

                return real_name;
            };
        }

        lua_pop(L, 1);

        params.on_removed = [=] {
            lua_freecallback(L, down_callback);
            lua_freecallback(L, up_callback);
            lua_freecallback(L, get_enabled);
            lua_freecallback(L, get_active);
            lua_freecallback(L, get_real_name);
        };

        return true;
    }

    static int add(lua_State* L)
    {
        auto lua = LuaManager::get_environment_for_state(L);

        ActionManager::t_action_params params;
        if (!check_action_params(L, params))
        {
            lua_pushboolean(L, false);
            return 1;
        }
        
        const auto result = ActionManager::add(params);

        if (result)
        {
            lua->registered_actions.emplace_back(params.path);
        }

        lua_pushboolean(L, result);
        return 1;
    }

    static int associate_hotkey(lua_State* L)
    {
        const auto path = lua_getwstring(L, 1);
        ::Hotkey::t_hotkey hotkey;
        if (!Hotkey::check_hotkey(L, 2, hotkey))
        {
            lua_pushboolean(L, false);
            return 1;
        }

        const auto result = ActionManager::associate_hotkey(path, hotkey);

        lua_pushboolean(L, result);
        return 1;
    }

    static int begin_batch_work(lua_State* L)
    {
        ActionManager::begin_batch_work();
        return 0;
    }

    static int end_batch_work(lua_State* L)
    {
        ActionManager::end_batch_work();
        return 0;
    }

    static int notify_enabled_changed(lua_State* L)
    {
        const auto path = lua_getwstring(L, 1);
        ActionManager::notify_enabled_changed(path);
        return 0;
    }

    static int notify_active_changed(lua_State* L)
    {
        const auto path = lua_getwstring(L, 1);
        ActionManager::notify_active_changed(path);
        return 0;
    }

    static int notify_real_name_changed(lua_State* L)
    {
        const auto path = lua_getwstring(L, 1);
        ActionManager::notify_real_name_changed(path);
        return 0;
    }

    static int get_display_name(lua_State* L)
    {
        const auto path = lua_getwstring(L, 1);
        const auto ignore_real_name = (bool)luaL_opt(L, lua_toboolean, 2, false);

        const auto result = ActionManager::get_display_name(path, ignore_real_name);

        lua_pushstring(L, io_service.wstring_to_string(result).c_str());
        return 1;
    }

    static int get_actions_matching_filter(lua_State* L)
    {
        const auto path = lua_getwstring(L, 1);

        const auto actions = ActionManager::get_actions_matching_filter(path);

        lua_newtable(L);
        size_t i = 1;
        for (const auto& action : actions)
        {
            lua_pushstring(L, io_service.wstring_to_string(action).c_str());
            lua_seti(L, -2, i++);
        }

        return 1;
    }

    static int invoke(lua_State* L)
    {
        const auto path = lua_getwstring(L, 1);
        const auto up = (bool)luaL_opt(L, lua_toboolean, 2, false);

        ActionManager::invoke(path, up);

        return 0;
    }
} // namespace LuaCore::Action
