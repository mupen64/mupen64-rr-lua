/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"

struct t_lua_helper_context
{
    std::unordered_map<void *, bool> valid_callback_tokens;
};

static t_lua_helper_context g_ctx;

uintptr_t *lua_optcallback(lua_State *L, int i)
{
    if (!lua_isfunction(L, i))
    {
        return nullptr;
    }

    const auto key = new uintptr_t();
    g_ctx.valid_callback_tokens[key] = true;

    lua_pushvalue(L, i);
    lua_pushlightuserdata(L, key);
    lua_pushvalue(L, -2);
    lua_settable(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);

    return key;
}

void lua_print_stack(lua_State *L)
{
    const int top = lua_gettop(L);
    for (int i = 1; i <= top; ++i)
    {
        std::wstring str = std::format(L"{}: ", IOUtils::to_wide_string(luaL_typename(L, i)));

        lua_getglobal(L, "tostringex");
        lua_pushvalue(L, i);
        lua_pcall(L, 1, 1, 0);
        const char *s = lua_tostring(L, -1);
        str += IOUtils::to_wide_string(s ? s : "(nil)");
        lua_pop(L, 1);

        g_view_logger->debug(L"stack[{}]: {}", i, str);
    }
}

uintptr_t *lua_tocallback(lua_State *L, const int i)
{
    if (!lua_isfunction(L, i))
    {
        luaL_error(L, "Expected a function at argument %d", i);
        return nullptr;
    }

    return lua_optcallback(L, i);
}

void lua_pushcallback(lua_State *L, uintptr_t *token, bool free)
{
    lua_pushlightuserdata(L, token);
    lua_gettable(L, LUA_REGISTRYINDEX);
    if (free)
    {
        lua_freecallback(L, token);
    }
}

void lua_freecallback(lua_State *L, uintptr_t *token)
{
    if (!g_ctx.valid_callback_tokens.contains(token))
    {
        return;
    }

    lua_pushlightuserdata(L, token);
    lua_pushnil(L);
    lua_settable(L, LUA_REGISTRYINDEX);

    g_ctx.valid_callback_tokens.erase(token);
    delete token;
}

std::wstring luaL_checkwstring(lua_State *L, int i)
{
    if (!lua_isstring(L, i))
    {
        luaL_error(L, "Expected a string at argument %d", i);
    }

    const auto str = lua_tostring(L, i);
    if (str == nullptr)
    {
        luaL_error(L, "Expected a string at argument %d", i);
    }

    return IOUtils::to_wide_string(str);
}

std::wstring luaL_optwstring(lua_State *L, int i, const std::wstring &def)
{
    if (lua_isnoneornil(L, i))
    {
        return def;
    }

    return luaL_checkwstring(L, i);
}

std::wstring lua_pushwstring(lua_State *L, const std::wstring &str)
{
    const auto s = IOUtils::to_utf8_string(str);
    lua_pushstring(L, s.c_str());
    return str;
}

bool luaL_checkboolean(lua_State *L, int i)
{
    if (!lua_isboolean(L, i))
    {
        luaL_error(L, "Expected a boolean at argument %d", i);
    }

    return lua_toboolean(L, i);
}