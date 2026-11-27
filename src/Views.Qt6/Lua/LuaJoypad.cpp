/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaInternal.hpp"
#include "EmuContext.hpp"

namespace LuaInternal
{

namespace
{

/** Returns the current controller state of a port, as a Lua table (joypad.get). */
void pushJoypad(lua_State *L, int port)
{
    CoreButtons buttons{};
    if (auto *ctx = EmuContext::instance())
    {
        buttons = ctx->luaControllerInput(port);
    }

    lua_newtable(L);
#define P(a, s)                                                                                                        \
    lua_pushboolean(L, buttons.a);                                                                                     \
    lua_setfield(L, -2, s)
    P(dr, "right");
    P(dl, "left");
    P(dd, "down");
    P(du, "up");
    P(start, "start");
    P(z, "Z");
    P(b, "B");
    P(a, "A");
    P(cr, "Cright");
    P(cl, "Cleft");
    P(cd, "Cdown");
    P(cu, "Cup");
    P(r, "R");
    P(l, "L");
#undef P
    lua_pushinteger(L, buttons.y);
    lua_setfield(L, -2, "Y");
    lua_pushinteger(L, buttons.x);
    lua_setfield(L, -2, "X");
}

/** Records a controller state from a Lua table, to be applied on the next input callback (joypad.set). */
void setJoypad(lua_State *L, int port)
{
    CoreButtons keys{};
#define P(a, s)                                                                                                        \
    lua_getfield(L, -1, s);                                                                                            \
    keys.a = static_cast<unsigned>(lua_toboolean(L, -1));                                                              \
    lua_pop(L, 1)
    P(dr, "right");
    P(dl, "left");
    P(dd, "down");
    P(du, "up");
    P(start, "start");
    P(z, "Z");
    P(b, "B");
    P(a, "A");
    P(cr, "Cright");
    P(cl, "Cleft");
    P(cd, "Cdown");
    P(cu, "Cup");
    P(r, "R");
    P(l, "L");
#undef P
    lua_getfield(L, -1, "Y");
    keys.y = static_cast<signed>(lua_tointeger(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "X");
    keys.x = static_cast<signed>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    if (auto *ctx = EmuContext::instance())
    {
        ctx->luaSetControllerInput(port, keys);
    }
}

int joypad_get(lua_State *L)
{
    int port = static_cast<int>(luaL_optinteger(L, 1, 1)) - 1;
    if (port < 0 || port >= 4) return luaL_error(L, "port: 1-4");
    pushJoypad(L, port);
    return 1;
}

int joypad_set(lua_State *L)
{
    int port;
    if (lua_type(L, 1) == LUA_TTABLE)
    {
        port = 0;
    }
    else
    {
        port = static_cast<int>(luaL_optinteger(L, 1, 1)) - 1;
    }
    if (port < 0 || port >= 4) return luaL_error(L, "port: 1-4");
    if (!lua_istable(L, -1)) return luaL_error(L, "table expected");
    setJoypad(L, port);
    return 0;
}

} // namespace

void registerJoypadAPIs(lua_State *L)
{
    lua_newtable(L);

    const luaL_Reg joypadFuncs[] = {
        {"get", joypad_get},
        {"set", joypad_set},
        {nullptr, nullptr},
    };

    luaL_setfuncs(L, joypadFuncs, 0);
    lua_setglobal(L, "joypad");
}

} // namespace LuaInternal
