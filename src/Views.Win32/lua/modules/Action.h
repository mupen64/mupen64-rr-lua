/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace LuaCore::Action
{
    static int add(lua_State* L)
    {
        // TODO: Implement
        lua_pushboolean(L, 1);
        return 1;
    }

    static int associate_hotkey(lua_State* L)
    {
        // TODO: Implement
        lua_pushboolean(L, 1);
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
