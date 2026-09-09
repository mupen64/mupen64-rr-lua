/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
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
 * \brief Gets the string at the given index in the Lua stack. Errors if the value is not a string or not present.
 * \param L The Lua state.
 * \param i The index of the value in the Lua stack.
 * \return The string at the given index in the Lua stack.
 */
std::string luaL_checkstlstring(lua_State *L, int i);

/**
 * \brief Gets the string at the given index in the Lua stack, or a default value if the value is not present or nil.
 * Errors if the value is not a string. \param L The Lua state. \param i The index of the value in the Lua stack.
 * \param def The default value to return if the value is not present or nil.
 * \return The string at the given index in the Lua stack, or the default value.
 */
std::string luaL_optstlstring(lua_State *L, int i, const std::string &def);

/**
 * \brief Pushes a string onto the Lua stack.
 * \param L The Lua state.
 * \param str The string to push.
 * \return The pushed string.
 */
std::string lua_pushstlstring(lua_State *L, const std::string &str);

/**
 * \brief Gets the boolean at the given index in the Lua stack. Errors if the value is not a boolean or not present.
 * \param L The Lua state.
 * \param i The index of the value in the Lua stack.
 * \return The boolean at the given index in the Lua stack.
 */
bool luaL_checkboolean(lua_State *L, int i);

/**
 * \brief Gets a finite number at the given index in the Lua stack. Errors if the value is not a number or not finite.
 * \param L The Lua state.
 * \param index The index of the value in the Lua stack.
 * \param name The name of the value, used in error messages.
 * \return The finite number at the given index in the Lua stack.
 */
float luaL_checkfinitenumber(lua_State *L, int index, const char *name);

/**
 * \brief Gets a numeric field from a table at the given index in the Lua stack, or a fallback value if the field is
 * not present or nil. Errors if the field is required and not present, or not a finite number.
 * \param L The Lua state.
 * \param table The index of the table in the Lua stack.
 * \param field The name of the field to read.
 * \param fallback The value to return if the field is not present or nil.
 * \param required Whether to error if the field is not present or nil.
 * \return The value of the field, or the fallback value.
 */
float luaL_tablenumber(lua_State *L, int table, const char *field, float fallback, bool required = false);

/**
 * \brief Gets a boolean field from a table at the given index in the Lua stack, or a fallback value if the field is
 * not present or nil.
 * \param L The Lua state.
 * \param table The index of the table in the Lua stack.
 * \param field The name of the field to read.
 * \param fallback The value to return if the field is not present or nil.
 * \return The value of the field, or the fallback value.
 */
bool luaL_tablebool(lua_State *L, int table, const char *field, bool fallback);

/**
 * \brief Gets a string field from a table at the given index in the Lua stack, or a fallback value if the field is
 * not present or nil. Errors if the field is present but not a string.
 * \param L The Lua state.
 * \param table The index of the table in the Lua stack.
 * \param field The name of the field to read.
 * \param fallback The value to return if the field is not present or nil.
 * \return The value of the field, or the fallback value.
 */
std::string luaL_tablestring(lua_State *L, int table, const char *field, const char *fallback);

/**
 * \brief Gets the string at the given index in the Lua stack, converted from UTF-8 to a wide string. Errors if the
 * value is not a string or not valid UTF-8.
 * \param L The Lua state.
 * \param i The index of the value in the Lua stack.
 * \return The wide string at the given index in the Lua stack.
 */
std::wstring luaL_checkstlwstring(lua_State *L, int i);

/**
 * \brief Creates a metatable with the given name and methods, and sets the index and gc functions.
 * \param L The Lua state.
 * \param name The name of the metatable.
 * \param methods The methods of the metatable.
 * \param index The index function of the metatable.
 * \param gc The gc function of the metatable.
 */
void luaL_create_metatable(
    lua_State *L, const char *name, const luaL_Reg *methods, lua_CFunction index, lua_CFunction gc);
