#pragma once

/*
 *	Interface for Lua-related calls originating from core layer to the view
 *layer
 *
 *	Must be implemented in the view layer.
 */

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <cstdint>
#include <shared/types/CoreTypes.h>

namespace LuaService
{
    /**
     * \brief Gets the last controller data for a controller index
     */
    BUTTONS get_last_controller_data(int index);

#pragma region Callbacks

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
     * \param input Pointer to the input data, can be modified by Lua scripts during
     * this function \param index The index of the controller being polled
     */
    void call_input(BUTTONS* input, int index);

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
     * \brief Notifies all lua instances of a warp modify operation's status
     * changing
     */
    void call_warp_modify_status_changed(int32_t status);

#pragma endregion

#pragma region Raw Calls
    int pcall_no_params(lua_State* L);
#pragma endregion
} // namespace LuaService
