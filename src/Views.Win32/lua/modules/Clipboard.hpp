/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaManager.hpp>

namespace LuaCore::Clipboard
{
const std::vector<std::pair<std::string, int32_t>> KNOWN_TYPES = {{"text", CF_TEXT}};

static int32_t validate_type(lua_State *L, const std::string &type)
{
    const auto it = std::ranges::find_if(KNOWN_TYPES, [&](const auto &pair) { return pair.first == type; });
    if (it == KNOWN_TYPES.end())
    {
        luaL_error(L, "Unknown clipboard type: %s", type.c_str());
    }
    return it->second;
}

static int get(lua_State *L)
{
    const auto type = luaL_checkstlstring(L, 1);

    const int32_t clipboard_type = validate_type(L, type);

    if (!IsClipboardFormatAvailable(clipboard_type))
    {
        lua_pushnil(L);
        return 1;
    }

    if (!OpenClipboard(nullptr))
    {
        lua_pushnil(L);
        return 1;
    }

    const HANDLE data = GetClipboardData(clipboard_type);
    if (!data)
    {
        CloseClipboard();
        lua_pushnil(L);
        return 1;
    }

    const void *cb_data = GlobalLock(data);
    if (!cb_data)
    {
        CloseClipboard();
        lua_pushnil(L);
        return 1;
    }

    if (type == "text")
    {
        lua_pushstring(L, static_cast<const char *>(cb_data));
    }
    else
    {
        std::unreachable();
    }

    GlobalUnlock(data);
    CloseClipboard();

    return 1;
}

static int get_content_type(lua_State *L)
{
    for (const auto &[name, id] : KNOWN_TYPES)
    {
        if (!IsClipboardFormatAvailable(id))
        {
            continue;
        }

        lua_pushstring(L, name.c_str());
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

static int set(lua_State *L)
{
    const auto type = luaL_checkstlstring(L, 1);

    const int32_t clipboard_type = validate_type(L, type);

    if (!OpenClipboard(g_main_ctx.hwnd))
    {
        lua_pushboolean(L, false);
        return 1;
    }

    if (!EmptyClipboard())
    {
        CloseClipboard();
        lua_pushboolean(L, false);
        return 1;
    }

    void *src_data;
    size_t src_data_size;

    if (type == "text")
    {
        const auto str = luaL_checkstlstring(L, 2);

        src_data_size = str.size() + 1;
        src_data = calloc(str.size() + 1, sizeof(char));
        memcpy(src_data, str.c_str(), str.size());
    }
    else
    {
        std::unreachable();
    }

    if (!src_data)
    {
        CloseClipboard();
        lua_pushboolean(L, false);
        return 1;
    }

    const HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, src_data_size);
    if (!hg)
    {
        CloseClipboard();
        lua_pushboolean(L, false);
        return 1;
    }

    void *dst = GlobalLock(hg);

    if (!dst)
    {
        GlobalFree(hg);
        CloseClipboard();
        lua_pushboolean(L, false);
        return 1;
    }

    memcpy(dst, src_data, src_data_size);
    free(src_data);

    GlobalUnlock(hg);

    if (!SetClipboardData(clipboard_type, hg))
    {
        GlobalFree(hg);
        CloseClipboard();
        lua_pushboolean(L, false);
        return 1;
    }

    CloseClipboard();

    lua_pushboolean(L, true);
    return 1;
}

static int clear(lua_State *L)
{
    if (!OpenClipboard(g_main_ctx.hwnd))
    {
        lua_pushboolean(L, false);
        return 1;
    }

    if (!EmptyClipboard())
    {
        CloseClipboard();
        lua_pushboolean(L, false);
        return 1;
    }

    CloseClipboard();

    lua_pushboolean(L, true);
    return 1;
}

} // namespace LuaCore::Clipboard
