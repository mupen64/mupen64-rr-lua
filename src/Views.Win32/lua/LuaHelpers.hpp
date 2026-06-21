/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief Prints the current Lua stack.
 */
void lua_print_stack(lua_State *L);

/**
 * \brief Converts a Lua function at the given index to a callback. Errors if the function is not a valid Lua function
 * or not present.
 * \param L The Lua state.
 * \param i The index of the function in the Lua stack.
 * \return A pointer to the callback token.
 */
uintptr_t *lua_tocallback(lua_State *L, int i);

/**
 * \brief Converts a Lua function at the given index to a callback.
 * \param L The Lua state.
 * \param i The index of the function in the Lua stack.
 * \return A pointer to the callback token, or nullptr if the function is not a valid Lua function or not present.
 */
uintptr_t *lua_optcallback(lua_State *L, int i);

/**
 * \brief Pushes a callback's Lua function onto the stack.
 * \param L The Lua state.
 * \param token A callback token.
 * \param free Whether to free the callback token after pushing it onto the stack. If true, the callback will be freed
 * after being pushed.
 */
void lua_pushcallback(lua_State *L, uintptr_t *token, bool free = true);

/**
 * \brief Frees a callback token from the Lua registry.
 * \param L The Lua state.
 * \param token A callback token.
 */
void lua_freecallback(lua_State *L, uintptr_t *token);

/**
 * \brief Gets the wide string at the given index in the Lua stack. Errors if the value is not a string or not present.
 * \param L The Lua state.
 * \param i The index of the value in the Lua stack.
 * \return The wide string at the given index in the Lua stack.
 */
std::wstring luaL_checkwstring(lua_State *L, int i);

/**
 * \brief Gets the wide string at the given index in the Lua stack, or a default value if the value is not present or
 * nil. Errors if the value is not a string. \param L The Lua state. \param i The index of the value in the Lua stack.
 * \param def The default value to return if the value is not present or nil.
 * \return The wide string at the given index in the Lua stack, or the default value.
 */
std::wstring luaL_optwstring(lua_State *L, int i, const std::wstring &def);

/**
 * \brief Pushes a wide string onto the Lua stack.
 * \param L The Lua state.
 * \param str The wide string to push.
 * \return The pushed wide string.
 */
std::wstring lua_pushwstring(lua_State *L, const std::wstring &str);

/**
 * \brief Gets the boolean at the given index in the Lua stack. Errors if the value is not a boolean or not present.
 * \param L The Lua state.
 * \param i The index of the value in the Lua stack.
 * \return The boolean at the given index in the Lua stack.
 */
bool luaL_checkboolean(lua_State *L, int i);
