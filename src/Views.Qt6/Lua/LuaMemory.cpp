/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaInternal.hpp"
#include "EmuContext.hpp"
#include <Core/API.hpp>
#include <cstdint>

#include <lua.hpp>


namespace LuaInternal
{

namespace
{

template <typename T> int memory_read(lua_State *L)
{
    uint32_t addr = static_cast<uint32_t>(luaL_checkinteger(L, 1));
    auto *ctx = EmuContext::rawContext();
    if (!ctx || !ctx->rdram)
    {
        return luaL_error(L, "Core or RDRAM not available");
    }
    auto val = core_rdram_load<T>((uint8_t *)ctx->rdram, addr);
    if (!val.has_value())
    {
        return luaL_error(L, "address out of bounds");
    }
    if constexpr (std::is_floating_point_v<T>)
    {
        lua_pushnumber(L, static_cast<lua_Number>(val.value()));
    }
    else
    {
        lua_pushinteger(L, static_cast<lua_Integer>(val.value()));
    }
    return 1;
}

template <typename T> int memory_write(lua_State *L)
{
    uint32_t addr = static_cast<uint32_t>(luaL_checkinteger(L, 1));
    T val{};
    if constexpr (std::is_floating_point_v<T>)
    {
        val = static_cast<T>(luaL_checknumber(L, 2));
    }
    else
    {
        val = static_cast<T>(luaL_checkinteger(L, 2));
    }

    auto *ctx = EmuContext::rawContext();
    if (!ctx || !ctx->rdram)
    {
        return luaL_error(L, "Core or RDRAM not available");
    }
    bool ok = core_rdram_store<T>((uint8_t *)ctx->rdram, addr, val);
    if (!ok)
    {
        return luaL_error(L, "address out of bounds");
    }
    return 0;
}

int memory_read_qword(lua_State *L)
{
    uint32_t addr = static_cast<uint32_t>(luaL_checkinteger(L, 1));
    auto *ctx = EmuContext::rawContext();
    if (!ctx || !ctx->rdram)
    {
        return luaL_error(L, "Core or RDRAM not available");
    }
    auto val = core_rdram_load<uint64_t>((uint8_t *)ctx->rdram, addr);
    if (!val.has_value())
    {
        return luaL_error(L, "address out of bounds");
    }

    // push uint64_t result to table
    uint64_t val_data = val.value();
    lua_createtable(L, 2, 0);
    // set element 1
    lua_pushinteger(L, 1);
    lua_pushinteger(L, static_cast<uint32_t>(val_data));
    lua_settable(L, -3);
    // set element 2
    lua_pushinteger(L, 2);
    lua_pushinteger(L, static_cast<uint32_t>(val_data >> 32));
    lua_settable(L, -3);
    return 1;
}

int memory_write_qword(lua_State *L)
{
    uint32_t addr = static_cast<uint32_t>(luaL_checkinteger(L, 1));

    // read uint64_t from table
    uint64_t val = 0;
    // read element 1
    lua_pushinteger(L, 1);
    lua_gettable(L, 2);
    val = static_cast<uint64_t>(static_cast<uint32_t>(luaL_checkinteger(L, -1)));
    lua_pop(L, 1);
    // read element 2
    lua_pushinteger(L, 2);
    lua_gettable(L, 2);
    val |= static_cast<uint64_t>(static_cast<uint32_t>(luaL_checkinteger(L, -1))) << 32;
    lua_pop(L, 1);

    auto *ctx = EmuContext::rawContext();
    if (!ctx || !ctx->rdram)
    {
        return luaL_error(L, "Core or RDRAM not available");
    }
    bool ok = core_rdram_store<uint64_t>((uint8_t *)ctx->rdram, addr, val);
    if (!ok)
    {
        return luaL_error(L, "address out of bounds");
    }
    return 0;
}
} // namespace

void registerMemoryAPIs(lua_State *L)
{
    lua_newtable(L);

    const luaL_Reg memFuncs[] = {
        {"readbyte", memory_read<uint8_t>},
        {"readbytesigned", memory_read<int8_t>},
        {"readword", memory_read<uint16_t>},
        {"readwordsigned", memory_read<int16_t>},
        {"readdword", memory_read<uint32_t>},
        {"readdwordsigned", memory_read<int32_t>},
        {"readqword", memory_read_qword},
        {"readqwordsigned", memory_read_qword},
        {"readfloat", memory_read<float>},
        {"readdouble", memory_read<double>},
        {"writebyte", memory_write<uint8_t>},
        {"writeword", memory_write<uint16_t>},
        {"writedword", memory_write<uint32_t>},
        {"writeqword", memory_write_qword},
        {"writefloat", memory_write<float>},
        {"writedouble", memory_write<double>},
        {nullptr, nullptr},
    };

    luaL_setfuncs(L, memFuncs, 0);
    lua_setglobal(L, "memory");
}

} // namespace LuaInternal
