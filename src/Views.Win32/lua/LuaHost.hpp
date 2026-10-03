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
    LuaHost();

    friend std::function<int(lua_State *)> get_function_for_callback(uint8_t key);

    std::vector<std::shared_ptr<LuaEnvironment>> m_environments;
    std::unordered_map<lua_State *, LuaEnvironment *> m_environment_by_state;

    struct AtwindowmessageContext
    {
        void *wnd;
        unsigned int msg;
        std::uintptr_t w_param;
        std::intptr_t l_param;
    };

    std::unordered_map<uint8_t, std::atomic<size_t>> m_callback_count_map;
    AtwindowmessageContext m_atwindowmessage_ctx{};
    LuaKeyEventArgs m_atkey_ctx{};
    LuaMouseEventArgs m_atmouse_ctx{};
    int m_current_input_n = 0;

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

    /**
     * \brief Tries to create a new Lua environment from the specified Lua script path and add it to the active
     * environment list.
     * \param path The path to the Lua script file.
     * \param destroying_callback The callback to invoke when the environment is destroyed.
     * \param print_callback The callback to invoke for printing messages.
     * \return A shared pointer to the new Lua environment, or an error message if creation fails.
     */
    std::expected<std::shared_ptr<LuaEnvironment>, std::string> create(
        const std::filesystem::path &path, const LuaDestroyFn &destroying_callback, const LuaPrintFn &print_callback);

    /**
     * \return All active Lua environments.
     */
    const std::vector<std::shared_ptr<LuaEnvironment>> &envs() const;

    /**
     * \return The Lua environment associated with the specified Lua state, or nullptr if none is found.
     */
    LuaEnvironment *get_by_state(lua_State *lua_state) const;

    /**
     * \brief Calls the specified callback key on the given Lua environment.
     * \param env The Lua environment to call the callback on.
     * \param key The callback key to invoke.
     * \return True if the callback was successfully invoked, false otherwise.
     */
    bool call_by_key(const LuaEnvironment *env, callback_key key);

    /**
     * \brief Calls the specified callback key on all active Lua environments.
     * \param key The callback key to invoke.
     */
    void call_by_key(callback_key key);

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
