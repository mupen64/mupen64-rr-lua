/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaInternal.hpp"

namespace LuaInternal
{

namespace
{

int lua_global_print(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst) return 0;

    int n = lua_gettop(L);
    QString out;
    for (int i = 1; i <= n; ++i)
    {
        if (i > 1) out += '\t';
        size_t len = 0;
        const char *s = luaL_tolstring(L, i, &len);
        if (s) out += QString::fromUtf8(s, static_cast<qsizetype>(len));
        lua_pop(L, 1);
    }
    out += '\n';
    inst->appendLog(out);
    return 0;
}

} // namespace

void registerGlobalAPIs(lua_State *L)
{
    lua_register(L, "print", lua_global_print);
    lua_register(L, "console", lua_global_print);
}

} // namespace LuaInternal
