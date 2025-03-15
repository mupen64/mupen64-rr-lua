#pragma once

/**
 * \brief A module responsible for implementing functionality related to the Lua function registry.
 */
namespace LuaRegistry
{
    /**
     * \brief Registers the standard Lua functions along with Mupen's functions to the specified state.
     */
    void register_functions(lua_State* L);
}
