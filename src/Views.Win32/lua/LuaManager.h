/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

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
     * \param trusted Whether the Lua environment is running in trusted mode. See sandbox.lua for more details.
     * \return The newly created lua environment or an error message if the operation failed.
     */
    std::expected<t_lua_environment*, std::wstring> create_environment(const std::filesystem::path& path, bool trusted, const std::function<void()>& destroyed_callback, const std::function<void(const std::wstring& path)>& print_callback);

    /**
     * \brief Destroys a lua environment.
     */
    void destroy_environment(t_lua_environment*);

} // namespace LuaManager


void* lua_tocallback(lua_State* L, int i);
void lua_pushcallback(lua_State* L, void* key);

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
