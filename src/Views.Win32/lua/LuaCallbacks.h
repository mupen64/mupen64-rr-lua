/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua.h>

/**
 * \brief A module responsible for implementing Lua callbacks.
 */
namespace LuaCallbacks
{

// The Lua Reference Manual specifically advises against this, but this should
// prevent reserved integer registry keys from being used.
enum callback_key : uint8_t {
    REG_LUACLASS = LUA_RIDX_LAST + 1,
    REG_ATUPDATESCREEN,
    REG_ATDRAWD2D,
    REG_ATVI,
    REG_ATINPUT,
    REG_ATSTOP,
    REG_SYNCBREAK,
    REG_READBREAK,
    REG_WRITEBREAK,
    REG_WINDOWMESSAGE,
    REG_ATINTERVAL,
    REG_ATPLAYMOVIE,
    REG_ATSTOPMOVIE,
    REG_ATLOADSTATE,
    REG_ATSAVESTATE,
    REG_ATRESET,
    REG_ATSEEKCOMPLETED,
    REG_ATWARPMODIFYSTATUSCHANGED,
};

/**
 * \brief Gets the last controller data for a controller index
 */
core_buttons get_last_controller_data(int index);

/**
 * \brief Notifies all lua instances of a window message
 */
void call_window_message(void *, unsigned int, unsigned int, long);

/**
 * \brief Notifies all lua instances of a visual interrupt
 */
void call_vi();

/**
 * \brief Notifies all lua instances of an input poll
 * \param input Pointer to the input data, can be modified by Lua scripts during this function
 * \param index The index of the controller being polled
 */
void call_input(core_buttons *input, int index);

/**
 * \brief Notifies all lua instances of the heartbeat while paused
 */
void call_interval();

/**
 * \brief Notifies all lua instances of movie playback starting
 */
void call_play_movie();

/**
 * \brief Notifies all lua instances of movie playback stopping
 */
void call_stop_movie();

/**
 * \brief Notifies all lua instances of a state being saves
 */
void call_save_state();

/**
 * \brief Notifies all lua instances of a state being loaded
 */
void call_load_state();

/**
 * \brief Notifies all lua instances of the rom being reset
 */
void call_reset();

/**
 * \brief Notifies all lua instances of a seek operation completing
 */
void call_seek_completed();

/**
 * \brief Notifies all lua instances of a warp modify operation's status changing
 */
void call_warp_modify_status_changed(int32_t status);

/**
 * \brief Invokes the registered callbacks with the specified key on the specified Lua environment.
 * \param lua The Lua environment.
 * \param key The callback key.
 * \return Whether the invocation failed.
 */
bool invoke_callbacks_with_key(const t_lua_environment *lua, callback_key key);

/**
 * \brief Invokes the registered callbacks with the specified key on all Lua instances in the global map.
 * \param key The callback key.
 */
void invoke_callbacks_with_key_on_all_instances(callback_key key);

/**
 * \brief Subscribes to or unsubscribes from the specified callback based on the input parameters.
 * If the second value on the Lua stack is true, the function is unregistered. Otherwise, it is registered.
 * \param l The Lua state.
 * \param key The callback key.
 */
void register_or_unregister_function(lua_State *l, callback_key key);
} // namespace LuaCallbacks
