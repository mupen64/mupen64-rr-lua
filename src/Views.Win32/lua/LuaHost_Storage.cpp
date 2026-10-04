/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/IDialogService.hpp>
#include <lua/LuaHost.hpp>
#include <lua/LuaRenderer.hpp>

LuaHost &LuaHost::instance()
{
    static LuaHost host;
    return host;
}

void LuaHost::rebuild_environment_map()
{
    m_environment_by_state.clear();
    for (const auto &env : m_environments)
    {
        m_environment_by_state[env->l()] = env.get();
    }
}

const std::vector<std::shared_ptr<LuaEnvironment>> &LuaHost::envs() const
{
    return m_environments;
}

LuaEnvironment *LuaHost::get_by_state(lua_State *lua_state) const
{
    const auto it = m_environment_by_state.find(lua_state);
    return it == m_environment_by_state.end() ? nullptr : it->second;
}

void LuaHost::add_environment(std::shared_ptr<LuaEnvironment> env)
{
    m_environments.push_back(std::move(env));
    rebuild_environment_map();
}

void LuaHost::remove_environment(const LuaEnvironment *env)
{
    std::erase_if(m_environments, [env](const auto &active) { return active.get() == env; });
    rebuild_environment_map();
}

std::expected<std::shared_ptr<LuaEnvironment>, std::string> LuaHost::create(const std::filesystem::path &path,
    const LuaDestroyFn &destroying_callback, const LuaPrintFn &print_callback)
{
    need(is_on_gui_thread(), "not on GUI thread");

    auto env = std::make_shared<LuaEnvironment>(path, destroying_callback, print_callback);

    lua_atpanic(env->l(), [](lua_State *L) {
        const char *raw_msg = lua_tostring(L, -1);
        const std::string_view message = raw_msg ? raw_msg : "";
        DialogService::show_dialog(message, "Lua", CoreMessageTone::Error);
        return 0;
    });
    LuaHost::instance().register_functions(env->l());
    env->renderer.initialize();

    return env;
}
