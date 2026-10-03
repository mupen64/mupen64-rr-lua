/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

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
    LuaHost(const LuaHost &) = delete;
    LuaHost &operator=(const LuaHost &) = delete;
    LuaHost(LuaHost &&) = delete;
    LuaHost &operator=(LuaHost &&) = delete;

    static LuaHost &instance();

    std::expected<std::shared_ptr<LuaEnvironment>, std::string> create(const std::filesystem::path &path,
        const LuaDestroyFn &destroying_callback, const LuaPrintFn &print_callback);
    const std::vector<std::shared_ptr<LuaEnvironment>> &envs() const;
    LuaEnvironment *get_by_state(lua_State *lua_state) const;
};
