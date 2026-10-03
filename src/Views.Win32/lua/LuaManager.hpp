/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaTypes.hpp>
#include <lua.h>

using LuaDestroyFn = std::function<void(const LuaEnvironment *env)>;
using LuaPrintFn = std::function<void(const LuaEnvironment *env, const std::string &text)>;

/**
 * \brief Describes a Lua instance.
 */
class LuaEnvironment : public std::enable_shared_from_this<LuaEnvironment>
{
  private:
    lua_State *m_l;
    std::filesystem::path m_path;
    bool m_started;

  public:
    LuaEnvironment(const std::filesystem::path &path, LuaDestroyFn destroying, LuaPrintFn print);
    ~LuaEnvironment();

    static std::expected<std::shared_ptr<LuaEnvironment>, std::string> create(
        const std::filesystem::path &path, const LuaDestroyFn &destroying_callback, const LuaPrintFn &print_callback);
    static const std::vector<std::shared_ptr<LuaEnvironment>> &envs();
    static LuaEnvironment *get_by_state(lua_State *lua_state);

    LuaEnvironment(const LuaEnvironment &) = delete;
    LuaEnvironment &operator=(const LuaEnvironment &) = delete;
    LuaEnvironment(LuaEnvironment &&) = delete;
    LuaEnvironment &operator=(LuaEnvironment &&) = delete;

    std::expected<void, std::string> start(bool trusted);
    void stop();

    lua_State *l() const { return m_l; }
    std::filesystem::path path() const { return m_path; }

    LuaRenderingContext rctx;
    std::vector<ActionManager::action_path> registered_actions;
    std::unordered_map<std::string, std::vector<ActionParamMeta>> param_meta_map;
    std::vector<std::pair<CoreBreakpointId, uintptr_t *>> active_breakpoints;
    std::vector<uintptr_t *> step_callbacks;
    LuaDestroyFn destroying;
    LuaPrintFn print;
};

/**
 * \brief The modified control data to be pushed the next frame
 */
extern CoreButtons g_new_controller_data[4];

/**
 * \brief Whether the <c>new_controller_data</c> of a controller should be pushed the next frame
 */
extern bool g_overwrite_controller_data[4];

/**
 * \brief Amount of call_input calls.
 */
extern size_t g_input_count;
