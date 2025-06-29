/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

constexpr uint32_t LUA_GDI_COLOR_MASK = RGB(255, 0, 255);
static HBRUSH g_alpha_mask_brush = CreateSolidBrush(LUA_GDI_COLOR_MASK);

/**
 * \brief Initializes the lua subsystem
 */
void lua_init();

/**
 * \brief Creates a lua environment, adding it to the global map and running it if the operation succeeds
 * \param path The script path
 * \param inst_wnd_ctx The associated context.
 * \return An error string, or an empty string if the operation succeeded
 */
std::string create_lua_environment(const std::filesystem::path& path, t_lua_wnd_ctx* inst_wnd_ctx, const std::function<void(const std::wstring& path)>& print_callback);

/**
 * \brief Stops, destroys and removes a lua environment from the environment map
 */
void destroy_lua_environment(t_lua_environment*);

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
