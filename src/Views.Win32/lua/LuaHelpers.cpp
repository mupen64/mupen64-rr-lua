/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"

struct LuaHelperContext
{
    std::unordered_map<void *, bool> valid_callback_tokens;
};

static LuaHelperContext g_ctx;

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
        std::string str = std::format("{}: ", luaL_typename(L, i));

        lua_getglobal(L, "tostringex");
        lua_pushvalue(L, i);
        lua_pcall(L, 1, 1, 0);
        const char *s = lua_tostring(L, -1);
        str += s ? s : "(nil)";
        lua_pop(L, 1);

        g_view_logger->debug("stack[{}]: {}", i, str);
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

std::string luaL_checkstlstring(lua_State *L, int i)
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

    return str;
}

std::string luaL_optstlstring(lua_State *L, int i, const std::string &def)
{
    if (lua_isnoneornil(L, i))
    {
        return def;
    }

    return luaL_checkstlstring(L, i);
}

std::string lua_pushstlstring(lua_State *L, const std::string &str)
{
    lua_pushstring(L, str.c_str());
    return str;
}

std::wstring luaL_checkstlwstring(lua_State *L, const int i)
{
    const auto text = luaL_checkstlstring(L, i);
    if (text.empty()) return {};

    const auto result = IOUtils::to_wide_string(text);
    if (result.empty()) luaL_error(L, "string is not valid UTF-8");
    return result;
}

bool luaL_checkboolean(lua_State *L, int i)
{
    if (!lua_isboolean(L, i))
    {
        luaL_error(L, "Expected a boolean at argument %d", i);
    }

    return lua_toboolean(L, i);
}

float luaL_checkfinitenumber(lua_State *L, const int index, const char *name)
{
    const auto value = static_cast<float>(luaL_checknumber(L, index));
    if (!std::isfinite(value)) luaL_error(L, "%s must be finite", name);
    return value;
}

float luaL_tablenumber(lua_State *L, int table, const char *field, const float fallback, const bool required)
{
    table = lua_absindex(L, table);
    lua_getfield(L, table, field);
    float result = fallback;
    if (lua_isnil(L, -1))
    {
        if (required) luaL_error(L, "field '%s' is required", field);
    }
    else
    {
        result = luaL_checkfinitenumber(L, -1, field);
    }
    lua_pop(L, 1);
    return result;
}

bool luaL_tablebool(lua_State *L, int table, const char *field, const bool fallback)
{
    table = lua_absindex(L, table);
    lua_getfield(L, table, field);
    const bool result = lua_isnil(L, -1) ? fallback : lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);
    return result;
}

std::string luaL_tablestring(lua_State *L, int table, const char *field, const char *fallback)
{
    table = lua_absindex(L, table);
    lua_getfield(L, table, field);
    std::string result = fallback;
    if (!lua_isnil(L, -1)) result = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    return result;
}

void luaL_create_metatable(
    lua_State *L, const char *name, const luaL_Reg *methods, lua_CFunction index, lua_CFunction gc)
{
    if (luaL_newmetatable(L, name))
    {
        luaL_setfuncs(L, methods, 0);
        if (index)
        {
            lua_pushcfunction(L, index);
            lua_setfield(L, -2, "__index");
        }
        else
        {
            lua_pushvalue(L, -1);
            lua_setfield(L, -2, "__index");
        }
        lua_pushcfunction(L, gc);
        lua_setfield(L, -2, "__gc");
        lua_pushstring(L, name);
        lua_setfield(L, -2, "__name");
    }
    lua_pop(L, 1);
}
