/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaTypes.h>

namespace LuaManager
{
    /**
     * \brief Initializes the lua subsystem.
     */
    void init();

    /**
     * \brief Gets the Lua environment associated with a Lua state, or nullptr if none exists.
     */
    t_lua_environment* get_environment_for_state(lua_State* lua_state);

    /**
     * \brief Creates a lua environment.
     * \param path The script path.
     * \param destroying_callback A callback that is called when the Lua environment is destroyed.
     * \param print_callback A callback that is called when the Lua environment prints text.
     * \return The newly created lua environment or an error message if the operation failed.
     */
    std::expected<t_lua_environment*, std::wstring> create_environment(const std::filesystem::path& path, const t_lua_environment::destroying_func& destroying_callback, const t_lua_environment::print_func& print_callback);

    /**
     * \brief Begins code execution in the a Lua environment.
     * \param env The Lua environment to start.
     * \param trusted Whether the Lua environment is running in trusted mode. See sandbox.lua for more details.
     * \return An error message if the operation failed.
     * \details Environments can only be started once. If you wish to restart an environment, you create a new environment and start that.
     */
    std::expected<void, std::wstring> start_environment(t_lua_environment* env, bool trusted);

    /**
     * \brief Destroys a lua environment.
     */
    void destroy_environment(t_lua_environment*);

} // namespace LuaManager

/**
 * \brief Converts a Lua function at the given index to a callback. Errors if the function is not a valid Lua function or not present.
 * \param L The Lua state.
 * \param i The index of the function in the Lua stack.
 * \return A pointer to the callback token.
 */
uintptr_t* lua_tocallback(lua_State* L, int i);

/**
 * \brief Converts a Lua function at the given index to a callback.
 * \param L The Lua state.
 * \param i The index of the function in the Lua stack.
 * \return A pointer to the callback token, or nullptr if the function is not a valid Lua function or not present.
 */
uintptr_t* lua_optcallback(lua_State* L, int i);

/**
 * \brief Pushes a callback's Lua function onto the stack.
 * \param L The Lua state.
 * \param token A callback token.
 * \param free Whether to free the callback token after pushing it onto the stack. If true, the callback will be freed after being pushed.
 */
void lua_pushcallback(lua_State* L, uintptr_t* token, bool free = true);

/**
 * \brief Frees a callback token from the Lua registry.
 * \param L The Lua state.
 * \param token A callback token.
 */
void lua_freecallback(lua_State* L, uintptr_t* token);

/**
 * \brief Gets the wide string at the given index in the Lua stack. Errors if the value is not a string or not present.
 * \param L The Lua state.
 * \param i The index of the value in the Lua stack.
 * \return The wide string at the given index in the Lua stack.
 */
std::wstring luaL_checkwstring(lua_State* L, int i);


/**
 * \brief Gets the wide string at the given index in the Lua stack, or a default value if the value is not present or nil. Errors if the value is not a string.
 * \param L The Lua state.
 * \param i The index of the value in the Lua stack.
 * \param def The default value to return if the value is not present or nil.
 * \return The wide string at the given index in the Lua stack, or the default value.
 */
std::wstring luaL_optwstring(lua_State* L, int i, const std::wstring& def);

/**
 * \brief Pushes a wide string onto the Lua stack.
 * \param L The Lua state.
 * \param str The wide string to push.
 * \return The pushed wide string.
 */
std::wstring lua_pushwstring(lua_State* L, const std::wstring& str);

/**
 * \brief Gets the boolean at the given index in the Lua stack. Errors if the value is not a boolean or not present.
 * \param L The Lua state.
 * \param i The index of the value in the Lua stack.
 * \return The boolean at the given index in the Lua stack.
 */
bool luaL_checkboolean(lua_State* L, int i);

extern std::vector<t_lua_environment*> g_lua_environments;

/**
 * \brief The controller data at time of the last input poll
 */
extern core_buttons g_last_controller_data[4];

/**
 * \brief The modified control data to be pushed the next frame
 */
extern core_buttons g_new_controller_data[4];

/**
 * \brief Whether the <c>new_controller_data</c> of a controller should be pushed the next frame
 */
extern bool g_overwrite_controller_data[4];

/**
 * \brief Amount of call_input calls.
 */
extern size_t g_input_count;
