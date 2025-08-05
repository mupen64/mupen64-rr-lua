/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <ActionManager.h>
#include <lua/LuaManager.h>

namespace LuaCore::Action
{
    static bool check_action_params(lua_State* L, ActionManager::t_action_params& params, std::function<void()>& free_params)
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
        // TODO: Implement

        lua_pop(L, 1);

        lua_getfield(L, 1, "get_enabled");
        auto get_enabled = lua_tocallback(L, -1);
        // TODO: Implement

        lua_pop(L, 1);

        lua_getfield(L, 1, "get_active");
        auto get_active = lua_tocallback(L, -1);
        // TODO: Implement

        lua_pop(L, 1);

        lua_getfield(L, 1, "get_real_name");
        auto get_real_name = lua_tocallback(L, -1);
        // TODO: Implement

        lua_pop(L, 1);

        free_params = [=] {
            lua_freecallback(L, down_callback);
        };

        return true;
    }

    static int add(lua_State* L)
    {
        ActionManager::t_action_params params;
        std::function<void()> free_params;
        if (!check_action_params(L, params, free_params))
        {
            lua_pushboolean(L, false);
            return 1;
        }

        const auto result = ActionManager::add(params);

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
        // TODO: Implement
        return 0;
    }

    static int end_batch_work(lua_State* L)
    {
        // TODO: Implement
        return 0;
    }

    static int notify_enabled_changed(lua_State* L)
    {
        // TODO: Implement
        return 0;
    }

    static int notify_active_changed(lua_State* L)
    {
        // TODO: Implement
        return 0;
    }

    static int notify_real_name_changed(lua_State* L)
    {
        // TODO: Implement
        return 0;
    }

    static int get_display_name(lua_State* L)
    {
        // TODO: Implement
        lua_pushstring(L, "");
        return 1;
    }

    static int get_actions_matching_filter(lua_State* L)
    {
        // TODO: Implement
        lua_newtable(L);
        return 1;
    }

    static int invoke(lua_State* L)
    {
        // TODO: Implement
        lua_newtable(L);
        return 1;
    }
} // namespace LuaCore::Action
