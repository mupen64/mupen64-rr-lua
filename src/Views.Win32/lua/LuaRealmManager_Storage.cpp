/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <Common.Views/IDialogService.hpp>
#include <lua/LuaRealmManager.hpp>
#include <lua/LuaRendererManager.hpp>

LuaRealmManager &LuaRealmManager::instance()
{
    static LuaRealmManager host;
    return host;
}

void LuaRealmManager::rebuild_realm_map()
{
    m_realm_by_state.clear();
    for (const auto &env : m_realms)
    {
        m_realm_by_state[env->l()] = env.get();
    }
}

const std::vector<std::shared_ptr<LuaRealm>> &LuaRealmManager::realms() const
{
    return m_realms;
}

LuaRealm *LuaRealmManager::get_by_state(lua_State *lua_state) const
{
    const auto it = m_realm_by_state.find(lua_state);
    return it == m_realm_by_state.end() ? nullptr : it->second;
}

void LuaRealmManager::add_realm(std::shared_ptr<LuaRealm> env)
{
    m_realms.push_back(std::move(env));
    rebuild_realm_map();
}

void LuaRealmManager::remove_realm(const LuaRealm *env)
{
    std::erase_if(m_realms, [env](const auto &active) { return active.get() == env; });
    rebuild_realm_map();
}

void LuaRealmManager::add(const std::shared_ptr<LuaRealm> &env)
{
    need(is_on_gui_thread(), "not on GUI thread");
    need(env != nullptr, "cannot add a null Lua realm");

    lua_atpanic(env->l(), [](lua_State *L) {
        const char *raw_msg = lua_tostring(L, -1);
        const std::string_view message = raw_msg ? raw_msg : "";
        DialogService::show_dialog(message, "Lua", CoreMessageTone::Error);
        return 0;
    });
    LuaRealmManager::instance().register_functions(env->l());
    env->renderer.initialize();
}
