/*
 * Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <ActionManager.h>
#include <lua/LuaManager.h>
#include <lua/modules/Hotkey.h>

namespace LuaCore::Action
{
static std::pair<ActionManager::t_action_param, t_action_param_meta> check_action_param(lua_State *L, int index)
{
    if (lua_gettop(L) < 1 || !lua_istable(L, index))
    {
        luaL_error(L, "Expected a table at argument 1");
        std::unreachable();
    }

    ActionManager::t_action_param param{};
    t_action_param_meta meta{};

    lua_getfield(L, index, "key");
    param.key = luaL_checkwstring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "name");
    param.name = luaL_checkwstring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "validator");
    meta.validator = lua_optcallback(L, -1);
    lua_pop(L, 1);

    if (meta.validator)
    {
        param.validator = [=](std::wstring_view value) -> std::optional<std::wstring> {
            if (!LuaManager::get_environment_for_state(L))
            {
                return std::nullopt;
            }

            lua_pushcallback(L, meta.validator, false);
            lua_pushwstring(L, std::wstring(value));
            lua_pcall(L, 1, 1, 0);

            if (lua_isstring(L, -1))
            {
                const auto error_msg = luaL_checkwstring(L, -1);
                lua_pop(L, 1);
                return error_msg;
            }
            else
            {
                lua_pop(L, 1);
                return std::nullopt;
            }
        };
    }

    lua_getfield(L, index, "get_initial_value");
    meta.get_initial_value = lua_optcallback(L, -1);
    lua_pop(L, 1);

    if (meta.get_initial_value)
    {
        param.get_initial_value = [=]() -> std::wstring {
            if (!LuaManager::get_environment_for_state(L))
            {
                return L"";
            }

            lua_pushcallback(L, meta.get_initial_value, false);
            lua_pcall(L, 0, 1, 0);

            const auto initial_value = luaL_checkwstring(L, -1);
            lua_pop(L, 1);
            return initial_value;
        };
    }

    lua_getfield(L, index, "get_hints");
    meta.get_hints = lua_optcallback(L, -1);
    lua_pop(L, 1);

    if (meta.get_hints)
    {
        param.get_hints = [=](const std::wstring_view input) -> std::vector<std::wstring> {
            if (!LuaManager::get_environment_for_state(L))
            {
                return {};
            }

            lua_pushcallback(L, meta.get_hints, false);
            lua_pushwstring(L, std::wstring(input));
            lua_pcall(L, 1, 1, 0);

            std::vector<std::wstring> hints;
            if (lua_istable(L, -1))
            {
                const int hints_table_index = lua_gettop(L);
                const int hints_count = luaL_len(L, hints_table_index);
                for (int i = 1; i <= hints_count; ++i)
                {
                    lua_geti(L, hints_table_index, i);
                    const auto hint = luaL_checkwstring(L, -1);
                    hints.push_back(hint);
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
            return hints;
        };
    }

    return {param, meta};
}

static void push_action_params(lua_State *L, const ActionManager::action_argument_map &params)
{
    lua_newtable(L);
    for (const auto &[key, value] : params)
    {
        lua_pushwstring(L, key);
        lua_pushwstring(L, value);
        lua_settable(L, -3);
    }
}

static ActionManager::action_argument_map check_action_param_list(lua_State *L, int index)
{
    ActionManager::action_argument_map params;

    if (!lua_istable(L, index))
    {
        luaL_error(L, "Expected a table at argument %d", index);
    }

    if (index < 0) index = lua_gettop(L) + index + 1;

    lua_pushnil(L);
    while (lua_next(L, index) != 0)
    {

        if (!lua_isstring(L, -2) || !lua_isstring(L, -1))
        {
            luaL_error(L, "Action parameter table must contain string-to-string entries");
        }

        const auto key = IOUtils::to_wide_string(lua_tostring(L, -2));
        const auto value = IOUtils::to_wide_string(lua_tostring(L, -1));

        params.emplace(key, value);

        lua_pop(L, 1);
    }

    return params;
}

static std::pair<ActionManager::t_action_add_params, std::vector<t_action_param_meta>> check_action_add_params(
    lua_State *L, int index)
{
    if (lua_gettop(L) < 1 || !lua_istable(L, index))
    {
        luaL_error(L, "Expected a table at argument 1");
        std::unreachable();
    }

    ActionManager::t_action_add_params params{};

    lua_getfield(L, 1, "path");
    params.path = luaL_checkwstring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "params");
    std::vector<t_action_param_meta> params_metas{};
    if (!lua_isnil(L, -1))
    {
        if (!lua_istable(L, -1))
        {
            luaL_error(L, "Expected 'params' field to be a table");
            std::unreachable();
        }

        const int params_table_index = lua_gettop(L);
        const int params_count = luaL_len(L, params_table_index);
        for (int i = 1; i <= params_count; ++i)
        {
            lua_geti(L, params_table_index, i);
            int element_idx = lua_gettop(L);

            auto [param, meta] = check_action_param(L, element_idx);
            params.params.push_back(param);
            params_metas.push_back(meta);

            lua_pop(L, 1);
        }
    }

    lua_pop(L, 1);

    lua_getfield(L, 1, "on_press");

    auto on_press = lua_optcallback(L, -1);
    if (on_press)
    {
        params.on_press = [=](const ActionManager::action_argument_map &params) {
            if (!LuaManager::get_environment_for_state(L))
            {
                return;
            }

            lua_pushcallback(L, on_press, false);
            push_action_params(L, params);
            lua_pcall(L, 1, 0, 0);
        };
    }

    lua_pop(L, 1);

    lua_getfield(L, 1, "on_release");

    auto on_release = lua_optcallback(L, -1);
    if (on_release)
    {
        params.on_release = [=] {
            if (!LuaManager::get_environment_for_state(L))
            {
                return;
            }

            lua_pushcallback(L, on_release, false);
            lua_pcall(L, 0, 0, 0);
        };
    }

    lua_pop(L, 1);

    lua_getfield(L, 1, "get_display_name");

    auto get_display_name = lua_optcallback(L, -1);
    if (get_display_name)
    {
        params.get_display_name = [=] -> std::wstring {
            if (!LuaManager::get_environment_for_state(L))
            {
                return L"";
            }

            lua_pushcallback(L, get_display_name, false);
            lua_pcall(L, 0, 1, 0);

            const auto display_name = luaL_checkwstring(L, -1);

            return display_name;
        };
    }

    lua_pop(L, 1);

    lua_getfield(L, 1, "get_enabled");

    auto get_enabled = lua_optcallback(L, -1);
    if (get_enabled)
    {
        params.get_enabled = [=] -> bool {
            if (!LuaManager::get_environment_for_state(L))
            {
                return false;
            }

            lua_pushcallback(L, get_enabled, false);
            lua_pcall(L, 0, 1, 0);

            bool enabled = false;
            if (lua_isboolean(L, -1))
            {
                enabled = lua_toboolean(L, -1);
                lua_pop(L, 1);
            }

            return enabled;
        };
    }

    lua_pop(L, 1);

    lua_getfield(L, 1, "get_active");

    auto get_active = lua_optcallback(L, -1);
    if (get_active)
    {
        params.get_active = [=] -> bool {
            if (!LuaManager::get_environment_for_state(L))
            {
                return false;
            }

            lua_pushcallback(L, get_active, false);
            lua_pcall(L, 0, 1, 0);

            bool active = false;
            if (lua_isboolean(L, -1))
            {
                active = lua_toboolean(L, -1);
                lua_pop(L, 1);
            }

            return active;
        };
    }

    lua_pop(L, 1);

    params.on_removed = [=] {
        lua_freecallback(L, on_press);
        lua_freecallback(L, on_release);
        lua_freecallback(L, get_enabled);
        lua_freecallback(L, get_active);
        lua_freecallback(L, get_display_name);

        for (const auto &meta : params_metas)
        {
            lua_freecallback(L, meta.validator);
            lua_freecallback(L, meta.get_initial_value);
            lua_freecallback(L, meta.get_hints);
        }
    };

    return {params, params_metas};
}

static int add(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto [params, meta] = check_action_add_params(L, 1);

    const auto result = ActionManager::add(params);

    if (result)
    {
        const auto normalized_path = ActionManager::normalize_filter(params.path);
        lua->registered_actions.emplace_back(normalized_path);
        lua->param_meta_map[normalized_path] = meta;
    }

    lua_pushboolean(L, result);
    return 1;
}

static int remove(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto filter = luaL_checkwstring(L, 1);

    const auto removed_actions = ActionManager::remove(filter);

    lua_newtable(L);
    size_t i = 1;
    for (const auto &action : removed_actions)
    {
        std::erase_if(lua->registered_actions,
                      [&](const auto &registered_action) { return registered_action == action; });
        lua->param_meta_map.erase(action);
        lua_pushstring(L, IOUtils::to_utf8_string(action).c_str());
        lua_seti(L, -2, i++);
    }

    return 1;
}

static int associate_hotkey(lua_State *L)
{
    const auto path = luaL_checkwstring(L, 1);
    const auto hotkey = Hotkey::check_hotkey(L, 2);
    const auto overwrite_existing = (bool)luaL_opt(L, lua_toboolean, 3, false);

    const auto result = ActionManager::associate_hotkey(path, hotkey, overwrite_existing);

    lua_pushboolean(L, result);
    return 1;
}

static int begin_batch_work(lua_State *L)
{
    ActionManager::begin_batch_work();
    return 0;
}

static int end_batch_work(lua_State *L)
{
    ActionManager::end_batch_work();
    return 0;
}

static int notify_display_name_changed(lua_State *L)
{
    const auto filter = luaL_checkwstring(L, 1);
    ActionManager::notify_display_name_changed(filter);
    return 0;
}

static int notify_enabled_changed(lua_State *L)
{
    const auto filter = luaL_checkwstring(L, 1);
    ActionManager::notify_enabled_changed(filter);
    return 0;
}

static int notify_active_changed(lua_State *L)
{
    const auto filter = luaL_checkwstring(L, 1);
    ActionManager::notify_active_changed(filter);
    return 0;
}

static int get_display_name(lua_State *L)
{
    const auto filter = luaL_checkwstring(L, 1);
    const auto ignore_override = (bool)luaL_opt(L, lua_toboolean, 2, false);

    const auto result = ActionManager::get_display_name(filter, ignore_override);

    lua_pushstring(L, IOUtils::to_utf8_string(result).c_str());
    return 1;
}

static int get_enabled(lua_State *L)
{
    const auto path = luaL_checkwstring(L, 1);

    const auto result = ActionManager::get_enabled(path);

    lua_pushboolean(L, result);
    return 1;
}

static int get_active(lua_State *L)
{
    const auto path = luaL_checkwstring(L, 1);

    const auto result = ActionManager::get_active(path);

    lua_pushboolean(L, result);
    return 1;
}

static int get_activatability(lua_State *L)
{
    const auto path = luaL_checkwstring(L, 1);

    const auto result = ActionManager::get_activatability(path);

    lua_pushboolean(L, result);
    return 1;
}

static int get_params(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);

    const auto path = luaL_checkwstring(L, 1);

    const auto params = ActionManager::get_params(path);
    const auto normalized_path = ActionManager::normalize_filter(path);

    const auto& meta_it = lua->param_meta_map[normalized_path];
    
    lua_newtable(L);
    size_t i = 1;
    for (const auto &param : params)
    {
        const auto& meta = meta_it[i - 1];

        lua_newtable(L);

        lua_pushwstring(L, param.key);
        lua_setfield(L, -2, "key");

        lua_pushwstring(L, param.name);
        lua_setfield(L, -2, "name");

        lua_pushcallback(L, meta.validator, false);
        lua_setfield(L, -2, "validator");

        lua_pushcallback(L, meta.get_initial_value, false);
        lua_setfield(L, -2, "get_initial_value");

        lua_pushcallback(L, meta.get_hints, false);
        lua_setfield(L, -2, "get_hints");
        
        lua_seti(L, -2, i++);
    }

    return 1;
}

static int get_actions_matching_filter(lua_State *L)
{
    const auto filter = luaL_checkwstring(L, 1);
    const auto actions = ActionManager::get_actions_matching_filter(filter);

    lua_newtable(L);
    size_t i = 1;
    for (const auto &action : actions)
    {
        lua_pushstring(L, IOUtils::to_utf8_string(action).c_str());
        lua_seti(L, -2, i++);
    }

    return 1;
}

static int invoke(lua_State *L)
{
    const auto path = luaL_checkwstring(L, 1);
    const auto up = (bool)luaL_opt(L, lua_toboolean, 2, false);
    const auto release_on_repress = (bool)luaL_opt(L, lua_toboolean, 3, true);
    const auto params = lua_isnoneornil(L, 4) ? ActionManager::action_argument_map{} : check_action_param_list(L, 4);

    ActionManager::invoke(path, up, release_on_repress, params);

    return 0;
}

static int lock_hotkeys(lua_State *L)
{
    const bool lock = luaL_checkboolean(L, 1);

    ActionManager::lock_hotkeys(lock);

    return 0;
}

static int get_hotkeys_locked(lua_State *L)
{
    const bool locked = ActionManager::get_hotkeys_locked();

    lua_pushboolean(L, locked);
    return 1;
}
} // namespace LuaCore::Action
