/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/**
 * \brief A module responsible for implementing Lua callbacks.
 */
namespace LuaCallbacks
{
    using callback_key = uint8_t;

    constexpr callback_key REG_LUACLASS = 1;
    constexpr callback_key REG_ATUPDATESCREEN = 2;
    constexpr callback_key REG_ATDRAWD2D = 3;
    constexpr callback_key REG_ATVI = 4;
    constexpr callback_key REG_ATINPUT = 5;
    constexpr callback_key REG_ATSTOP = 6;
    constexpr callback_key REG_SYNCBREAK = 7;
    constexpr callback_key REG_READBREAK = 8;
    constexpr callback_key REG_WRITEBREAK = 9;
    constexpr callback_key REG_WINDOWMESSAGE = 10;
    constexpr callback_key REG_ATINTERVAL = 11;
    constexpr callback_key REG_ATPLAYMOVIE = 12;
    constexpr callback_key REG_ATSTOPMOVIE = 13;
    constexpr callback_key REG_ATLOADSTATE = 14;
    constexpr callback_key REG_ATSAVESTATE = 15;
    constexpr callback_key REG_ATRESET = 16;
    constexpr callback_key REG_ATSEEKCOMPLETED = 17;
    constexpr callback_key REG_ATWARPMODIFYSTATUSCHANGED = 18;

    /**
     * \brief Gets the last controller data for a controller index
     */
    core_buttons get_last_controller_data(int index);

    /**
     * \brief Notifies all lua instances of a window message
     */
    void call_window_message(void*, unsigned int, unsigned int, long);

    /**
     * \brief Notifies all lua instances of a visual interrupt
     */
    void call_vi();

    /**
     * \brief Notifies all lua instances of an input poll
     * \param input Pointer to the input data, can be modified by Lua scripts during this function
     * \param index The index of the controller being polled
     */
    void call_input(core_buttons* input, int index);

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
    bool invoke_callbacks_with_key(const t_lua_environment& lua, callback_key key);

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
    void register_or_unregister_function(lua_State* l, callback_key key);
} // namespace LuaCallbacks
