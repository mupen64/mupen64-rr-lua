/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace LuaCore::Global
{
static int print(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const int nargs = lua_gettop(L);

    for (int i = 1; i <= nargs; i++)
    {
        lua_getglobal(L, "__mupeninspect");
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            lua_pushstring(L, "__mupeninspect not in global scope");
            lua_error(L);
        }

        lua_getfield(L, -1, "inspect");

        lua_pushvalue(L, i);
        lua_pcall(L, 1, 1, 0);

        const char *inspected_value = lua_tostring(L, -1);
        if (inspected_value)
        {
            auto str = IOUtils::to_wide_string(inspected_value);

            // inspect puts quotes around strings, even when they're not nested in a table. We want to remove those...
            if (str.size() > 1 &&
                ((str[0] == '"' && str[str.size() - 1] == '"') || (str[0] == '\'' && str[str.size() - 1] == '\'')))
            {
                str = str.substr(1, str.size() - 2);
            }

            lua->print(lua, str);
        }
        else
        {
            lua->print(lua, L"???");
        }
        lua_pop(L, 2);

        if (i < nargs) lua->print(lua, L"\t");
    }

    lua->print(lua, L"\r\n");
    return 0;
}

static int tostringexs(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const int nargs = lua_gettop(L);

    std::wstring final_str;

    for (int i = 1; i <= nargs; i++)
    {
        lua_getglobal(L, "__mupeninspect");
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            lua_pushstring(L, "__mupeninspect not in global scope");
            lua_error(L);
        }

        lua_getfield(L, -1, "inspect");

        lua_pushvalue(L, i);

        lua_newtable(L);
        lua_pushstring(L, "");
        lua_setfield(L, -2, "newline");

        lua_pcall(L, 2, 1, 0);

        const char *inspected_value = lua_tostring(L, -1);
        if (inspected_value)
        {
            auto str = IOUtils::to_wide_string(inspected_value);

            // inspect puts quotes around strings, even when they're not nested in a table. We want to remove those...
            if (str.size() > 1 && str[0] == '"' && str[str.size() - 1] == '"')
            {
                str = str.substr(1, str.size() - 2);
            }

            final_str += str;
        }
        else
        {
            final_str += L"???";
        }
        lua_pop(L, 2);

        if (i < nargs) final_str += L"\t";
    }

    lua_pushstring(L, IOUtils::to_utf8_string(final_str).c_str());
    return 1;
}

static int StopScript(lua_State *L)
{
    luaL_error(L, "Stop requested");
    return 0;
}

// NOTE: The default os.exit implementation calls C++ destructors before closing the main window (WM_CLOSE +
// WM_DESTROY), thereby ripping the program apart for the remaining section of time until the exit, which causes
// extremely unpredictable crashes and an impossible program state. We therefore use our own softer os.exit.
static int Exit(lua_State *L)
{
    // FIXME: Exit-code and close params are ignored
    PostMessage(g_main_ctx.hwnd, WM_CLOSE, 0, 0);
    return 0;
}
} // namespace LuaCore::Global
