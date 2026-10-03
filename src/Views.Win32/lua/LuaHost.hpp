/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Core/Types.hpp>
#include <lua/LuaEnvironment.hpp>

class LuaHost
{
  private:
    LuaHost() = default;

    std::vector<std::shared_ptr<LuaEnvironment>> m_environments;
    std::unordered_map<lua_State *, LuaEnvironment *> m_environment_by_state;

    void rebuild_environment_map();
    void add_environment(std::shared_ptr<LuaEnvironment> env);
    void remove_environment(const LuaEnvironment *env);

    friend class LuaEnvironment;

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

    LuaHost(const LuaHost &) = delete;
    LuaHost &operator=(const LuaHost &) = delete;
    LuaHost(LuaHost &&) = delete;
    LuaHost &operator=(LuaHost &&) = delete;

    static LuaHost &instance();

    std::expected<std::shared_ptr<LuaEnvironment>, std::string> create(
        const std::filesystem::path &path, const LuaDestroyFn &destroying_callback, const LuaPrintFn &print_callback);
    const std::vector<std::shared_ptr<LuaEnvironment>> &envs() const;
    LuaEnvironment *get_by_state(lua_State *lua_state) const;

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

    bool call_by_key(const LuaEnvironment *env, callback_key key);
    void call_by_key(callback_key key);

    void register_or_unregister_function(lua_State *state, callback_key key);
    void unregister_all(lua_State *state);
};
