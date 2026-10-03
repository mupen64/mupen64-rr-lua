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

    std::filesystem::path path;
    LuaRenderingContext rctx;
    bool started{};

    // All the actions registered by the script. Stored so we can remove them when the script is destroyed.
    std::vector<ActionManager::action_path> registered_actions{};

    std::unordered_map<std::string, std::vector<ActionParamMeta>> param_meta_map;

    // All the breakpoints registered by the script. Stored so we can remove them when the script is destroyed.
    std::vector<std::pair<CoreBreakpointId, uintptr_t *>> active_breakpoints;

    std::vector<uintptr_t *> step_callbacks;

    LuaDestroyFn destroying{};
    LuaPrintFn print{};
};
