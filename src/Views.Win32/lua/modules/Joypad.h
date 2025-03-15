/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace LuaCore::Joypad
{
    static int lua_get_joypad(lua_State* L)
    {
        int i = luaL_optinteger(L, 1, 1) - 1;
        if (i < 0 || i >= 4)
        {
            luaL_error(L, "port: 1-4");
        }
        lua_newtable(L);
#define A(a,s) lua_pushboolean(L,last_controller_data[i].a);lua_setfield(L, -2, s)
        A(dr, "right");
        A(dl, "left");
        A(dd, "down");
        A(du, "up");
        A(start, "start");
        A(z, "Z");
        A(b, "B");
        A(a, "A");
        A(cr, "Cright");
        A(cl, "Cleft");
        A(cd, "Cdown");
        A(cu, "Cup");
        A(r, "R");
        A(l, "L");
#undef A
        lua_pushinteger(L, last_controller_data[i].x);
        lua_setfield(L, -2, "Y");
        lua_pushinteger(L, last_controller_data[i].y);
        lua_setfield(L, -2, "X");
        return 1;
    }

    static int lua_set_joypad(lua_State* L)
    {
        int a_2 = 2;
        int i;
        if (lua_type(L, 1) == LUA_TTABLE)
        {
            a_2 = 1;
            i = 0;
        }
        else
        {
            i = luaL_optinteger(L, 1, 1) - 1;
        }
        if (i < 0 || i >= 4)
        {
            luaL_error(L, "control: 1-4");
        }
        lua_pushvalue(L, a_2);
#define A(a,s) lua_getfield(L, -1, s);new_controller_data[i].a=lua_toboolean(L,-1);lua_pop(L,1)
        A(dr, "right");
        A(dl, "left");
        A(dd, "down");
        A(du, "up");
        A(start, "start");
        A(z, "Z");
        A(b, "B");
        A(a, "A");
        A(cr, "Cright");
        A(cl, "Cleft");
        A(cd, "Cdown");
        A(cu, "Cup");
        A(r, "R");
        A(l, "L");
        lua_getfield(L, -1, "Y");
        new_controller_data[i].x = lua_tointeger(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "X");
        new_controller_data[i].y = lua_tointeger(L, -1);
        lua_pop(L, 1);
        overwrite_controller_data[i] = true;
#undef A
        return 1;
    }
}
