/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/IDialogService.hpp>
#include <lua/LuaEnvironmentManager.hpp>
#include <lua/LuaRendererManager.hpp>

LuaEnvironmentManager &LuaEnvironmentManager::instance()
{
    static LuaEnvironmentManager host;
    return host;
}

void LuaEnvironmentManager::rebuild_environment_map()
{
    m_environment_by_state.clear();
    for (const auto &env : m_environments)
    {
        m_environment_by_state[env->l()] = env.get();
    }
}

const std::vector<std::shared_ptr<LuaEnvironment>> &LuaEnvironmentManager::envs() const
{
    return m_environments;
}

LuaEnvironment *LuaEnvironmentManager::get_by_state(lua_State *lua_state) const
{
    const auto it = m_environment_by_state.find(lua_state);
    return it == m_environment_by_state.end() ? nullptr : it->second;
}

void LuaEnvironmentManager::add_environment(std::shared_ptr<LuaEnvironment> env)
{
    m_environments.push_back(std::move(env));
    rebuild_environment_map();
}

void LuaEnvironmentManager::remove_environment(const LuaEnvironment *env)
{
    std::erase_if(m_environments, [env](const auto &active) { return active.get() == env; });
    rebuild_environment_map();
}

void LuaEnvironmentManager::add(const std::shared_ptr<LuaEnvironment> &env)
{
    need(is_on_gui_thread(), "not on GUI thread");
    need(env != nullptr, "cannot add a null Lua environment");

    lua_atpanic(env->l(), [](lua_State *L) {
        const char *raw_msg = lua_tostring(L, -1);
        const std::string_view message = raw_msg ? raw_msg : "";
        DialogService::show_dialog(message, "Lua", CoreMessageTone::Error);
        return 0;
    });
    LuaEnvironmentManager::instance().register_functions(env->l());
    env->renderer.initialize();
}
