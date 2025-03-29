/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace LuaCore::Global
{
    static int print(lua_State* L)
    {
        auto lua = get_lua_class(L);

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

            const char* inspected_value = lua_tostring(L, -1);
            if (inspected_value)
            {
                auto str = string_to_wstring(inspected_value);

                // inspect puts quotes around strings, even when they're not nested in a table. We want to remove those...
                if (str.size() > 1 && ((str[0] == '"' && str[str.size() - 1] == '"') || (str[0] == '\'' && str[str.size() - 1] == '\'')))
                {
                    str = str.substr(1, str.size() - 2);
                }

                print_con(lua->hwnd, str);
            }
            else
            {
                print_con(lua->hwnd, L"???");
            }
            lua_pop(L, 2);

            if (i < nargs)
                print_con(lua->hwnd, L"\t");
        }

        print_con(lua->hwnd, L"\r\n");
        return 0;
    }

    static int tostringexs(lua_State* L)
    {
        auto lua = get_lua_class(L);

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

            const char* inspected_value = lua_tostring(L, -1);
            if (inspected_value)
            {
                auto str = string_to_wstring(inspected_value);

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

            if (i < nargs)
                final_str += L"\t";
        }

        lua_pushstring(L, wstring_to_string(final_str).c_str());
        return 1;
    }

    static int StopScript(lua_State* L)
    {
        luaL_error(L, "Stop requested");
        return 0;
    }

    static int Exit(lua_State* L)
    {
        // FIXME: Exit-code and close params are ignored
        PostMessage(g_main_hwnd, WM_CLOSE, 0, 0);
        return 0;
    }
} // namespace LuaCore::Global
