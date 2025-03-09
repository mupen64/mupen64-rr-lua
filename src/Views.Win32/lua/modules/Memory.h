/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


#include <gui/Main.h>

namespace LuaCore::Memory
{
    static DWORD LuaCheckIntegerU(lua_State* L, int i = -1)
    {
        return (DWORD)luaL_checknumber(L, i);
    }

    static ULONGLONG LuaCheckQWord(lua_State* L, int i)
    {
        lua_pushinteger(L, 1);
        lua_gettable(L, i);
        ULONGLONG n = (ULONGLONG)LuaCheckIntegerU(L) << 32;
        lua_pop(L, 1);
        lua_pushinteger(L, 2);
        lua_gettable(L, i);
        n |= LuaCheckIntegerU(L);
        return n;
    }

    static void LuaPushQword(lua_State* L, ULONGLONG x)
    {
        lua_newtable(L);
        lua_pushinteger(L, 1);
        lua_pushinteger(L, x >> 32);
        lua_settable(L, -3);
        lua_pushinteger(L, 2);
        lua_pushinteger(L, x & 0xFFFFFFFF);
        lua_settable(L, -3);
    }

    // Read functions

    static int LuaReadByteUnsigned(lua_State* L)
    {
        UCHAR value = core_rdram_load<UCHAR>((uint8_t*)g_core.rdram, luaL_checkinteger(L, 1));
        lua_pushinteger(L, value);
        return 1;
    }

    static int LuaReadByteSigned(lua_State* L)
    {
        CHAR value = core_rdram_load<CHAR>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1));
        lua_pushinteger(L, value);
        return 1;
    }

    static int LuaReadWordUnsigned(lua_State* L)
    {
        USHORT value = core_rdram_load<USHORT>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1));
        lua_pushinteger(L, value);
        return 1;
    }

    static int LuaReadWordSigned(lua_State* L)
    {
        SHORT value = core_rdram_load<SHORT>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1));
        lua_pushinteger(L, value);
        return 1;
    }

    static int LuaReadDWorldUnsigned(lua_State* L)
    {
        ULONG value = core_rdram_load<ULONG>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1));
        lua_pushinteger(L, value);
        return 1;
    }

    static int LuaReadDWordSigned(lua_State* L)
    {
        LONG value = core_rdram_load<LONG>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1));
        lua_pushinteger(L, value);
        return 1;
    }

    static int LuaReadQWordUnsigned(lua_State* L)
    {
        ULONGLONG value = core_rdram_load<ULONGLONG>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1));
        LuaPushQword(L, value);
        return 1;
    }

    static int LuaReadQWordSigned(lua_State* L)
    {
        LONGLONG value = core_rdram_load<LONGLONG>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1));
        LuaPushQword(L, value);
        return 1;
    }

    static int LuaReadFloat(lua_State* L)
    {
        ULONG value = core_rdram_load<ULONG>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1));
        lua_pushnumber(L, *(FLOAT*)&value);
        return 1;
    }

    static int LuaReadDouble(lua_State* L)
    {
        ULONGLONG value = core_rdram_load<ULONGLONG>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1));
        lua_pushnumber(L, *(DOUBLE*)value);
        return 1;
    }

    // Write functions

    static int LuaWriteByteUnsigned(lua_State* L)
    {
        core_rdram_store<UCHAR>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
        return 0;
    }

    static int LuaWriteWordUnsigned(lua_State* L)
    {
        core_rdram_store<USHORT>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
        return 0;
    }

    static int LuaWriteDWordUnsigned(lua_State* L)
    {
        core_rdram_store<ULONG>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
        return 0;
    }

    static int LuaWriteQWordUnsigned(lua_State* L)
    {
        core_rdram_store<ULONGLONG>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1), LuaCheckQWord(L, 2));
        return 0;
    }

    static int LuaWriteFloatUnsigned(lua_State* L)
    {
        FLOAT f = luaL_checknumber(L, -1);
        core_rdram_store<ULONG>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1), *(ULONG*)&f);
        return 0;
    }

    static int LuaWriteDoubleUnsigned(lua_State* L)
    {
        DOUBLE f = luaL_checknumber(L, -1);
        core_rdram_store<ULONGLONG>((uint8_t*)g_core.rdram,luaL_checkinteger(L, 1), *(ULONGLONG*)&f);
        return 0;
    }

    static int LuaReadSize(lua_State* L)
    {
        ULONG addr = luaL_checkinteger(L, 1);
        int size = luaL_checkinteger(L, 2);
        switch (size)
        {
        // unsigned
        case 1: lua_pushinteger(L, core_rdram_load<UCHAR>((uint8_t*)g_core.rdram,addr));
            break;
        case 2: lua_pushinteger(L, core_rdram_load<USHORT>((uint8_t*)g_core.rdram,addr));
            break;
        case 4: lua_pushinteger(L, core_rdram_load<ULONG>((uint8_t*)g_core.rdram,addr));
            break;
        case 8: LuaPushQword(L, core_rdram_load<ULONGLONG>((uint8_t*)g_core.rdram,addr));
            break;
        // signed
        case -1: lua_pushinteger(L, core_rdram_load<CHAR>((uint8_t*)g_core.rdram,addr));
            break;
        case -2: lua_pushinteger(L, core_rdram_load<SHORT>((uint8_t*)g_core.rdram,addr));
            break;
        case -4: lua_pushinteger(L, core_rdram_load<LONG>((uint8_t*)g_core.rdram,addr));
            break;
        case -8: LuaPushQword(L, core_rdram_load<LONGLONG>((uint8_t*)g_core.rdram,addr));
            break;
        default: luaL_error(L, "size must be 1, 2, 4, 8, -1, -2, -4, -8");
        }
        return 1;
    }

    static int LuaWriteSize(lua_State* L)
    {
        ULONG addr = luaL_checkinteger(L, 1);
        int size = luaL_checkinteger(L, 2);
        switch (size)
        {
        case 1: core_rdram_store<UCHAR>((uint8_t*)g_core.rdram,addr, luaL_checkinteger(L, 3));
            break;
        case 2: core_rdram_store<USHORT>((uint8_t*)g_core.rdram,addr, luaL_checkinteger(L, 3));
            break;
        case 4: core_rdram_store<ULONG>((uint8_t*)g_core.rdram,addr, luaL_checkinteger(L, 3));
            break;
        case 8: core_rdram_store<ULONGLONG>((uint8_t*)g_core.rdram,addr, LuaCheckQWord(L, 3));
            break;
        case -1: core_rdram_store<CHAR>((uint8_t*)g_core.rdram,addr, luaL_checkinteger(L, 3));
            break;
        case -2: core_rdram_store<SHORT>((uint8_t*)g_core.rdram,addr, luaL_checkinteger(L, 3));
            break;
        case -4: core_rdram_store<LONG>((uint8_t*)g_core.rdram,addr, luaL_checkinteger(L, 3));
            break;
        case -8: core_rdram_store<LONGLONG>((uint8_t*)g_core.rdram,addr, LuaCheckQWord(L, 3));
            break;
        default: luaL_error(L, "size must be 1, 2, 4, 8, -1, -2, -4, -8");
        }
        return 0;
    }

    static int LuaIntToFloat(lua_State* L)
    {
        ULONG n = luaL_checknumber(L, 1);
        lua_pushnumber(L, *(FLOAT*)&n);
        return 1;
    }

    static int LuaIntToDouble(lua_State* L)
    {
        ULONGLONG n = LuaCheckQWord(L, 1);
        lua_pushnumber(L, *(DOUBLE*)&n);
        return 1;
    }

    static int LuaFloatToInt(lua_State* L)
    {
        FLOAT n = luaL_checknumber(L, 1);
        lua_pushinteger(L, *(ULONG*)&n);
        return 1;
    }

    static int LuaDoubleToInt(lua_State* L)
    {
        DOUBLE n = luaL_checknumber(L, 1);
        LuaPushQword(L, *(ULONGLONG*)&n);
        return 1;
    }

    static int LuaQWordToNumber(lua_State* L)
    {
        ULONGLONG n = LuaCheckQWord(L, 1);
        lua_pushnumber(L, n);
        return 1;
    }

    template <typename T>
    static void PushT(lua_State* L, T value)
    {
        LuaPushIntU(L, value);
    }

    template <>
    static void PushT<ULONGLONG>(lua_State* L, ULONGLONG value)
    {
        LuaPushQword(L, value);
    }

    template <typename T>
    static T CheckT(lua_State* L, int i)
    {
        return LuaCheckIntegerU(L, i);
    }

    template <>
    static ULONGLONG CheckT<ULONGLONG>(lua_State* L, int i)
    {
        return LuaCheckQWord(L, i);
    }
}
