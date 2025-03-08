/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace LuaCore::Global
{
    static int ToStringExs(lua_State* L);

    static int Print(lua_State* L)
    {
        lua_pushcfunction(L, ToStringExs, "ToStringExs");
        lua_insert(L, 1);
        lua_call(L, lua_gettop(L) - 1, 1);
        get_lua_class(L)->print(string_to_wstring(lua_tostring(L, 1)) + L"\r\n");
        return 0;
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

    static int GetInfo(lua_State* L)
    {
        auto lua = get_lua_class(L);
        
        auto level = luaL_checkinteger(L, 1);
        auto str = luaL_optstring(L, 2, "");
        
        lua_newtable(L);
        lua_pushstring(L, "source");
        lua_pushstring(L, ("@" + lua->path.string()).c_str());
        lua_settable(L, -3);
        
        return 1;
    }

    static int DoFile(lua_State* L)
    {
        auto lua = get_lua_class(L);
        auto path = luaL_checkstring(L, 1);
        const auto code = read_file_string(path);
        
        if (code.empty())
        {
            lua_pushstring(L, "failed to load file");
            lua_error(L);
            return 1;
        }

        size_t bytecode_size = 0;
        char* bytecode = luau_compile(code.data(), code.size(), nullptr, &bytecode_size);
        int result = luau_load(L, path, bytecode, bytecode_size, 0);
        free(bytecode);

        // Whatever... to complicated since they removed loadfile/dofile        
        
        if (result)
        {
            size_t len;
            const char* msg = lua_tolstring(L, -1, &len);
            lua_pushstring(L, msg);
            lua_pop(L, 1);
            lua_error(L);
        }
        
        return 0;
    }

    static int BSHL(lua_State* L)
    {
        auto x = luaL_checkinteger(L, 1);
        auto y = luaL_checkinteger(L, 2);
        lua_pushinteger(L, x << y);
        return 1;
    }

    static int BSHR(lua_State* L)
    {
        auto x = luaL_checkinteger(L, 1);
        auto y = luaL_checkinteger(L, 2);
        lua_pushinteger(L, x >> y);
        return 1;
    }

    static int BOR(lua_State* L)
    {
        auto x = luaL_checkinteger(L, 1);
        auto y = luaL_checkinteger(L, 2);
        lua_pushinteger(L, x | y);
        return 1;
    }

    static int BAND(lua_State* L)
    {
        auto x = luaL_checkinteger(L, 1);
        auto y = luaL_checkinteger(L, 2);
        lua_pushinteger(L, x && y);
        return 1;
    }

    static int BXOR(lua_State* L)
    {
        auto x = luaL_checkinteger(L, 1);
        auto y = luaL_checkinteger(L, 2);
        lua_pushinteger(L, x ^ y);
        return 1;
    }
    
    static int ToStringEx(lua_State* L)
    {
        switch (lua_type(L, -1))
        {
        case LUA_TNIL:
        case LUA_TBOOLEAN:
        case LUA_TFUNCTION:
        case LUA_TUSERDATA:
        case LUA_TTHREAD:
        case LUA_TLIGHTUSERDATA:
        case LUA_TNUMBER:
            lua_getglobal(L, "tostring");
            lua_pushvalue(L, -2);
            lua_call(L, 1, 1);
            lua_insert(L, lua_gettop(L) - 1);
            lua_pop(L, 1);
            break;
        case LUA_TSTRING:
            lua_getglobal(L, "string");
            lua_getfield(L, -1, "format");
            lua_pushstring(L, "%q");
            lua_pushvalue(L, -4);
            lua_call(L, 2, 1);
            lua_insert(L, lua_gettop(L) - 2);
            lua_pop(L, 2);
            break;
        case LUA_TTABLE:
            {
                lua_pushvalue(L, -1);
                lua_rawget(L, 1);
                if (lua_toboolean(L, -1))
                {
                    lua_pop(L, 2);
                    lua_pushstring(L, "{...}");
                    return 1;
                }
                lua_pop(L, 1);
                lua_pushvalue(L, -1);
                lua_pushboolean(L, TRUE);
                lua_rawset(L, 1);
                int isArray = 0;
                std::string s("{");
                lua_pushnil(L);
                if (lua_next(L, -2))
                {
                    while (1)
                    {
                        lua_pushvalue(L, -2);
                        if (lua_type(L, -1) == LUA_TNUMBER &&
                            isArray + 1 == lua_tonumber(L, -1))
                        {
                            lua_pop(L, 1);
                            isArray++;
                        }
                        else
                        {
                            isArray = -1;
                            if (lua_type(L, -1) == LUA_TSTRING)
                            {
                                s.append(lua_tostring(L, -1));
                                lua_pop(L, 1);
                            }
                            else
                            {
                                ToStringEx(L);
                                s.append("[").append(lua_tostring(L, -1)).
                                  append("]");
                                lua_pop(L, 1);
                            }
                        }
                        ToStringEx(L);
                        if (isArray == -1)
                        {
                            s.append("=");
                        }
                        s.append(lua_tostring(L, -1));
                        lua_pop(L, 1);
                        if (!lua_next(L, -2))break;
                        s.append(", ");
                    }
                }
                lua_pop(L, 1);
                s.append("}");
                lua_pushstring(L, s.c_str());
                break;
            }
        }
        return 1;
    }

    static int ToStringExInit(lua_State* L)
    {
        lua_newtable(L);
        lua_insert(L, 1);
        ToStringEx(L);
        return 1;
    }

    static int ToStringExs(lua_State* L)
    {
        int len = lua_gettop(L);
        std::string str("");
        for (int i = 0; i < len; i++)
        {
            lua_pushvalue(L, 1 + i);
            if (lua_type(L, -1) != LUA_TSTRING)
            {
                ToStringExInit(L);
            }
            str.append(lua_tostring(L, -1));
            lua_pop(L, 1);
            if (i != len - 1) { str.append("\t"); }
        }
        lua_pushstring(L, str.c_str());
        return 1;
    }

    static int PrintX(lua_State* L)
    {
        int len = lua_gettop(L);
        std::string str("");
        for (int i = 0; i < len; i++)
        {
            lua_pushvalue(L, 1 + i);
            if (lua_type(L, -1) == LUA_TNUMBER)
            {
                int n = (DWORD)luaL_checknumber(L, -1);
                lua_pop(L, 1);
                lua_getglobal(L, "string");
                lua_getfield(L, -1, "format"); //string,string.format
                lua_pushstring(L, "%X"); //s,f,X
                lua_pushinteger(L, n); //s,f,X,n
                lua_call(L, 2, 1); //s,r
                lua_insert(L, lua_gettop(L) - 1); //
                lua_pop(L, 1);
            }
            else if (lua_type(L, -1) == LUA_TSTRING)
            {
                //do nothing
            }
            else
            {
                ToStringExInit(L);
            }
            str.append(lua_tostring(L, -1));
            lua_pop(L, 1);
            if (i != len - 1) { str.append("\t"); }
        }
        get_lua_class(L)->print(string_to_wstring(str) + L"\r\n");
        return 1;
    }
}
