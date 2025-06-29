/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

constexpr uint32_t LUA_GDI_COLOR_MASK = RGB(255, 0, 255);
static HBRUSH g_alpha_mask_brush = CreateSolidBrush(LUA_GDI_COLOR_MASK);

/**
 * A module responsible for implementing Lua script management functionality.
 */
namespace LuaManager
{
    /**
     * \brief Shows the lua manager dialog.
     */
    void show_manager_dialog();

    /**
     * \brief Adds a lua script to the lua manager and runs it. Also shows the lua manager dialog and selects the new script in the list.
     * \param path The path to the lua script to add and run.
     */
    void add_and_run(const std::filesystem::path& path);

    /**
     * \brief Saves all running lua scripts to an internal buffer.
     */
    void save_running_scripts();

    /**
     * \brief Recalls all running lua scripts from the internal buffer and starts them. Resets the internal buffer.
     */
    void recall_and_start_scripts();
} // namespace LuaManager


/**
 * \brief Initializes the lua subsystem
 */
void lua_init();

/**
 * \brief Stops all lua scripts
 */
void lua_stop_all_scripts();

/**
 * \brief Stops all lua scripts and removes all entries from the instance manager.
 */
void lua_close_all_scripts();

/**
 * \brief Creates a lua environment, adding it to the global map and running it if the operation succeeds
 * \param path The script path
 * \param inst_wnd_ctx The associated context.
 * \return An error string, or an empty string if the operation succeeded
 */
std::string create_lua_environment(const std::filesystem::path& path, t_lua_wnd_ctx* inst_wnd_ctx);

/**
 * \brief Stops, destroys and removes a lua environment from the environment map
 */
void destroy_lua_environment(t_lua_environment*);

/**
 * \brief Prints text to a lua environment.
 * \param env The lua environment to print to.
 * \param text The text to display.
 */
void print_con(const t_lua_environment& env, const std::wstring& text);

void* lua_tocallback(lua_State* L, int i);
void lua_pushcallback(lua_State* L, void* key);

extern std::vector<t_lua_environment*> g_lua_environments;

/**
 * \brief The controller data at time of the last input poll
 */
extern core_buttons last_controller_data[4];

/**
 * \brief The modified control data to be pushed the next frame
 */
extern core_buttons new_controller_data[4];

/**
 * \brief Whether the <c>new_controller_data</c> of a controller should be pushed the next frame
 */
extern bool overwrite_controller_data[4];

/**
 * \brief Amount of call_input calls.
 */
extern size_t g_input_count;

/**
 * \brief Gets the Lua environment associated with a lua state.
 */
t_lua_environment* get_lua_class(lua_State* lua_state);
