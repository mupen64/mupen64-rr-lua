/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <components/TextEditDialog.h>
#include <lua/LuaManager.h>

namespace LuaCore::Input
{
const char *KeyName[256] = {
    NULL,          "leftclick",   "rightclick",
    NULL,          "middleclick", NULL,
    NULL,          NULL,          "backspace",
    "tab",         NULL,          NULL,
    NULL,          "enter",       NULL,
    NULL,          "shift",       "control",
    "alt",
    "pause", // 0x10
    "capslock",    NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          "escape",      NULL,
    NULL,          NULL,          NULL,
    "space",       "pageup",      "pagedown",
    "end", // 0x20
    "home",        "left",        "up",
    "right",       "down",        NULL,
    NULL,          NULL,          NULL,
    "insert",      "delete",      NULL,
    "0",           "1",           "2",
    "3",           "4",           "5",
    "6",           "7",           "8",
    "9",           NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          "A",
    "B",           "C",           "D",
    "E",           "F",           "G",
    "H",           "I",           "J",
    "K",           "L",           "M",
    "N",           "O",           "P",
    "Q",           "R",           "S",
    "T",           "U",           "V",
    "W",           "X",           "Y",
    "Z",           NULL,          NULL,
    NULL,          NULL,          NULL,
    "numpad0",     "numpad1",     "numpad2",
    "numpad3",     "numpad4",     "numpad5",
    "numpad6",     "numpad7",     "numpad8",
    "numpad9",     "numpad*",     "numpad+",
    NULL,          "numpad-",     "numpad.",
    "numpad/",     "F1",          "F2",
    "F3",          "F4",          "F5",
    "F6",          "F7",          "F8",
    "F9",          "F10",         "F11",
    "F12",         "F13",         "F14",
    "F15",         "F16",         "F17",
    "F18",         "F19",         "F20",
    "F21",         "F22",         "F23",
    "F24",         NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    "numlock",     "scrolllock",
    NULL, // 0x92
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,
    NULL, // 0xB9
    "semicolon",   "plus",        "comma",
    "minus",       "period",      "slash",
    "tilde",
    NULL, // 0xC1
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,
    NULL, // 0xDA
    "leftbracket", "backslash",   "rightbracket",
    "quote",
};

static int get_keys(lua_State *L)
{
    lua_newtable(L);
    for (int i = 1; i < 255; i++)
    {
        const char *name = KeyName[i];
        if (name)
        {
            int active;
            if (i == VK_CAPITAL || i == VK_NUMLOCK || i == VK_SCROLL)
                active = GetKeyState(i) & 0x01;
            else
                active = GetAsyncKeyState(i) & 0x8000;
            if (active)
            {
                lua_pushboolean(L, true);
                lua_setfield(L, -2, name);
            }
        }
    }

    POINT mouse;
    GetCursorPos(&mouse);
    ScreenToClient(g_main_ctx.hwnd, &mouse);
    lua_pushinteger(L, mouse.x);
    lua_setfield(L, -2, "xmouse");
    lua_pushinteger(L, mouse.y);
    lua_setfield(L, -2, "ymouse");
    lua_pushinteger(L, g_main_ctx.last_wheel_delta > 0 ? 1 : (g_main_ctx.last_wheel_delta < 0 ? -1 : 0));
    lua_setfield(L, -2, "ywmouse");
    return 1;
}

/*
    local oinp
    emu.atvi(function()
        local inp = input.get()
        local dinp = input.diff(inp, oinp)
        ...
        oinp = inp
    end)
*/
static int GetKeyDifference(lua_State *L)
{
    if (lua_isnil(L, 1))
    {
        lua_newtable(L);
        lua_insert(L, 1);
    }
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_newtable(L);
    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        lua_pushvalue(L, -2);
        lua_gettable(L, 2);
        if (lua_isnil(L, -1))
        {
            lua_pushvalue(L, -3);
            lua_pushboolean(L, 1);
            lua_settable(L, 3);
        }
        lua_pop(L, 2);
    }
    return 1;
}

static int LuaGetKeyNameText(lua_State *L)
{
    const auto vk = luaL_checkinteger(L, 1);

    UINT scan_code = MapVirtualKeyEx(vk, MAPVK_VK_TO_VSC, GetKeyboardLayout(0));

    // Add extended bit to scan code for certain keys which have a two-byte form
    switch (vk)
    {
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_END:
    case VK_HOME:
    case VK_INSERT:
    case VK_DELETE:
    case VK_DIVIDE:
    case VK_NUMLOCK:
        scan_code |= 0x100;
        break;
    default:
        break;
    }

    TCHAR name[64]{};
    if (!GetKeyNameText(scan_code << 16, name, sizeof(name) / sizeof(TCHAR)))
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, IOUtils::to_utf8_string(name).c_str());
    return 1;
}

static int prompt(lua_State *L)
{
    const auto caption = luaL_optwstring(L, 1, L"input:");
    const auto text = luaL_optwstring(L, 2, L"");

    const auto result = TextEditDialog::show({.text = text, .caption = caption});

    if (result.has_value())
    {
        auto str = result.value();

        // COMPAT: \r\n is replaced with \n. Not sure why, but we're keeping this old implementation detail.
        std::string::size_type p = 0;
        while ((p = str.find(L"\r\n", p)) != std::string::npos)
        {
            str.replace(p, 2, L"\n");
            p += 1;
        }

        lua_pushwstring(L, str);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}
} // namespace LuaCore::Input
