/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaInternal.hpp"
#include "EmuContext.hpp"
#include <Core/API.hpp>

#include <type_traits>

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
        {"readfloat", memory_read<float>},
        {"readdouble", memory_read<double>},
        {"writebyte", memory_write<uint8_t>},
        {"writeword", memory_write<uint16_t>},
        {"writedword", memory_write<uint32_t>},
        {"writefloat", memory_write<float>},
        {"writedouble", memory_write<double>},
        {nullptr, nullptr},
    };

    luaL_setfuncs(L, memFuncs, 0);
    lua_setglobal(L, "memory");
}

} // namespace LuaInternal
