/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaManager.hpp>
#include <SDL3/SDL_clipboard.h>

namespace LuaCore::Clipboard
{
const std::vector<std::string> KNOWN_TYPES = {"text"};

static void validate_type(lua_State *L, const std::string &type)
{
    const auto it = std::ranges::find(KNOWN_TYPES, type);
    if (it == KNOWN_TYPES.end())
    {
        luaL_error(L, "Unknown clipboard type: %s", type.c_str());
    }
}

static int get(lua_State *L)
{
    const auto type = luaL_checkstlstring(L, 1);
    validate_type(L, type);

    if (!SDL_HasClipboardText())
    {
        lua_pushnil(L);
        return 1;
    }

    char *text = SDL_GetClipboardText();
    if (!text)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, text);
    SDL_free(text);

    return 1;
}

static int get_content_type(lua_State *L)
{
    if (SDL_HasClipboardText())
    {
        lua_pushstring(L, "text");
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

static int set(lua_State *L)
{
    const auto type = luaL_checkstlstring(L, 1);
    validate_type(L, type);

    const auto str = luaL_checkstlstring(L, 2);

    if (!SDL_SetClipboardText(str.c_str()))
    {
        lua_pushboolean(L, false);
        return 1;
    }

    lua_pushboolean(L, true);
    return 1;
}

static int clear(lua_State *L)
{
    if (!SDL_ClearClipboardData())
    {
        lua_pushboolean(L, false);
        return 1;
    }

    lua_pushboolean(L, true);
    return 1;
}

} // namespace LuaCore::Clipboard
