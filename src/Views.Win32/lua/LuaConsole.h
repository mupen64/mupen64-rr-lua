/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

constexpr uint32_t LUA_GDI_COLOR_MASK = RGB(255, 0, 255);
static HBRUSH g_alpha_mask_brush = CreateSolidBrush(LUA_GDI_COLOR_MASK);

static int pcall_no_params(lua_State* L)
{
    return lua_pcall(L, 0, 0, 0);
}

/**
 * \brief Initializes the lua subsystem
 */
void lua_init();

/**
 * \brief Creates a lua window and runs the specified script
 * \param path The script's path
 */
void lua_create_and_run(const std::wstring& path);

/**
 * \brief Creates a lua window
 * \return The window's handle
 */
HWND lua_create();

/**
 * \brief Stops all lua scripts
 */
void lua_stop_all_scripts();

/**
 * \brief Stops all lua scripts and closes their windows
 */
void lua_close_all_scripts();

/**
 * \brief Creates a lua environment, adding it to the global map and running it if the operation succeeds
 * \param path The script path
 * \param wnd The associated window
 * \return An error string, or an empty string if the operation succeeded
 */
std::string create_lua_environment(const std::filesystem::path& path, HWND wnd);

/**
 * \brief Stops, destroys and removes a lua environment from the environment map
 */
void destroy_lua_environment(t_lua_environment*);

/**
 * \brief Prints text to a lua environment dialog's console
 * \param hwnd Handle to a lua environment dialog of IDD_LUAWINDOW
 * \param text The text to display
 */
void print_con(HWND hwnd, const std::wstring& text);

void* lua_tocallback(lua_State* L, int i);
void lua_pushcallback(lua_State* L, void* key);
void lua_freecallback(void* key);

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
