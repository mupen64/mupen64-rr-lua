/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Core/Types.hpp>
#include <lua/LuaRealm.hpp>

class LuaRealmManager
{
  private:
    LuaRealmManager();

    std::vector<std::shared_ptr<LuaRealm>> m_realms;
    std::unordered_map<lua_State *, LuaRealm *> m_realm_by_state;

    std::unordered_map<uint8_t, std::atomic<size_t>> m_callback_count_map;

    void rebuild_realm_map();
    void add_realm(std::shared_ptr<LuaRealm> env);
    void remove_realm(const LuaRealm *env);
    void register_functions(lua_State *L);

    friend class LuaRealm;

  public:
    enum callback_key : uint8_t
    {
        REG_LUACLASS = LUA_RIDX_LAST + 1,
        REG_ATUPDATESCREEN,
        REG_ATPAINT,
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
        REG_ATKEY,
        REG_ATMOUSE,
        _COUNT,
    };

    LuaRealmManager(const LuaRealmManager &) = delete;
    LuaRealmManager &operator=(const LuaRealmManager &) = delete;
    LuaRealmManager(LuaRealmManager &&) = delete;
    LuaRealmManager &operator=(LuaRealmManager &&) = delete;

    static LuaRealmManager &instance();

    // The modified control data to be pushed the next frame.
    CoreButtons new_controller_data[4];

    // Whether the modified controller data should be pushed next frame.
    bool overwrite_controller_data[4];

    // Amount of call_input calls.
    size_t input_count = 0;

    /**
     * \brief Prepares an existing Lua realm for use by the manager.
     * \param env The realm to prepare.
     */
    void add(const std::shared_ptr<LuaRealm> &env);

    /**
     * \return All active Lua realms.
     */
    const std::vector<std::shared_ptr<LuaRealm>> &realms() const;

    /**
     * \return The Lua realm associated with the specified Lua state, or nullptr if none is found.
     */
    LuaRealm *get_by_state(lua_State *lua_state) const;

    /**
     * \brief Calls the specified callback key on the given Lua realm.
     * \param env The Lua realm to call the callback on.
     * \param key The callback key to invoke.
     * \param function Invokes the Lua function on the stack, supplying any event arguments.
     * \return True if the callback was successfully invoked, false otherwise.
     */
    bool call_by_key(
        const LuaRealm *env, callback_key key,
        const std::function<int(lua_State *)> &function = [](lua_State *l) { return lua_pcall(l, 0, 0, 0); });

    /**
     * \brief Calls the specified callback key on all active Lua realms.
     * \param key The callback key to invoke.
     * \param function Invokes the Lua function on the stack, supplying any event arguments.
     */
    void call_by_key(
        callback_key key,
        const std::function<int(lua_State *)> &function = [](lua_State *l) { return lua_pcall(l, 0, 0, 0); });

    void call_window_message(void *wnd, unsigned int msg, std::uintptr_t w, std::intptr_t l);
    void call_vi();
    void call_input(CoreButtons *input, int index);
    void call_interval();
    void call_play_movie();
    void call_stop_movie();
    void call_save_state();
    void call_load_state();
    void call_reset();
    void call_seek_completed();
    void call_warp_modify_status_changed(int32_t status);
    void call_atkey(const LuaKeyEventArgs &args);
    void call_atmouse(const LuaMouseEventArgs &args);
};
