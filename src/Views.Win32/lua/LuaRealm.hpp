/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaTypes.hpp>
#include <lua/LuaRenderer.hpp>
#include <lua.h>

class LuaRealm;

using LuaStoppingFn = std::function<void(const LuaRealm *env)>;
using LuaPrintFn = std::function<void(const LuaRealm *env, const std::string &text)>;

/**
 * \brief Describes a Lua instance.
 */
class LuaRealm : public std::enable_shared_from_this<LuaRealm>
{
  private:
    lua_State *m_l;
    std::filesystem::path m_path;
    bool m_started{};

    LuaRealm(const std::filesystem::path &path, LuaStoppingFn stopping, LuaPrintFn print);

  public:
    static std::shared_ptr<LuaRealm> create(
        const std::filesystem::path &path, LuaStoppingFn stopping, LuaPrintFn print);
    ~LuaRealm();

    LuaRealm(const LuaRealm &) = delete;
    LuaRealm &operator=(const LuaRealm &) = delete;
    LuaRealm(LuaRealm &&) = delete;
    LuaRealm &operator=(LuaRealm &&) = delete;

    /**
     * \brief Starts the Lua realm.
     * \param trusted Whether the realm is exempt from the Lua sandbox.
     * \return An error message if the realm could not be started, otherwise nothing.
     */
    std::expected<void, std::string> start(bool trusted);

    /**
     * \brief Stops the Lua realm.
     */
    void stop();

    /**
     * \brief Registers or unregisters a function in the Lua realm.
     * The Lua stack must contain the function at the top and the registration bool below it.
     * \param key The key of the function to register or unregister.
     */
    void register_or_unregister_function(uint8_t key);

    lua_State *l() const { return m_l; }
    std::filesystem::path path() const { return m_path; }

    LuaRenderer renderer;
    std::vector<ActionManager::action_path> registered_actions;
    std::unordered_map<std::string, std::vector<ActionParamMeta>> param_meta_map;
    std::vector<std::pair<CoreBreakpointId, uintptr_t *>> active_breakpoints;
    std::vector<uintptr_t *> step_callbacks;
    LuaStoppingFn stopping;
    LuaPrintFn print;
};
