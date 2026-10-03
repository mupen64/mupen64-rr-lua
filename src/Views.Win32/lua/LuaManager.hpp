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

public:
    LuaEnvironment(const std::filesystem::path &path, LuaDestroyFn destroying, LuaPrintFn print);
    ~LuaEnvironment();

    LuaEnvironment(const LuaEnvironment &) = delete;
    LuaEnvironment &operator=(const LuaEnvironment &) = delete;
    LuaEnvironment(LuaEnvironment &&) = delete;
    LuaEnvironment &operator=(LuaEnvironment &&) = delete;

    lua_State *l() const { return m_l; }
    std::expected<void, std::string> start(bool trusted);
    void stop();

    std::filesystem::path path;
    LuaRenderingContext rctx;
    bool started{};

    std::vector<ActionManager::action_path> registered_actions{};
    std::unordered_map<std::string, std::vector<ActionParamMeta>> param_meta_map;
    std::vector<std::pair<CoreBreakpointId, uintptr_t *>> active_breakpoints;
    std::vector<uintptr_t *> step_callbacks;

    LuaDestroyFn destroying{};
    LuaPrintFn print{};
};

namespace LuaManager
{
/**
 * \brief Initializes the lua subsystem.
 */
void init();

/**
 * \brief Gets the active Lua environments.
 */
const std::vector<std::shared_ptr<LuaEnvironment>> &envs();

/**
 * \brief Gets the Lua environment associated with a Lua state, or nullptr if none exists.
 */
LuaEnvironment *get_environment_for_state(lua_State *lua_state);

/**
 * \brief Creates a lua environment.
 * \param path The script path.
 * \param destroying_callback A callback that is called when the Lua environment is destroyed.
 * \param print_callback A callback that is called when the Lua environment prints text.
 * \return The newly created lua environment or an error message if the operation failed.
 */
std::expected<std::shared_ptr<LuaEnvironment>, std::string> create_environment(const std::filesystem::path &path,
    const LuaDestroyFn &destroying_callback, const LuaPrintFn &print_callback);


} // namespace LuaManager


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
