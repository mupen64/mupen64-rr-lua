/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaAction.hpp"
#include "LuaInstance.hpp"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>
#include <vector>

namespace
{
// A hotkey as accepted by action.associate_hotkey: either the legacy `key` field
// (Windows VKeycode, same values as pushKeyTable) or `trigger = { type = "keycode",
// value = SDL keycode }` (same values as the keycode2 field of event tables).
struct Hotkey
{
    bool valid{false};
    bool isSdl{false};
    int value{0};
    bool ctrl{false};
    bool shift{false};
    bool alt{false};
};

struct Action
{
    std::string path;
    int onPress{LUA_NOREF};
    int onRelease{LUA_NOREF};
    int getActive{LUA_NOREF};
    int getEnabled{LUA_NOREF};
    Hotkey hotkey;
    bool pressed{false}; // drives invoke()'s up/release_on_repress semantics
};

struct Registry
{
    LuaInstance *instance{nullptr};
    std::vector<std::unique_ptr<Action>> actions;
    bool hotkeysLocked{false};
    int lockedHotkeysRef{LUA_NOREF}; // the optional hotkey list passed to lock_hotkeys

    ~Registry()
    {
        if (!instance || !instance->state()) return;
        lua_State *L = instance->state();
        for (auto &action : actions)
        {
            for (int ref : {action->onPress, action->onRelease, action->getActive, action->getEnabled})
            {
                if (ref != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, ref);
            }
        }
        if (lockedHotkeysRef != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, lockedHotkeysRef);
    }

    Action *find(const std::string &path)
    {
        for (auto &action : actions)
        {
            if (action->path == path) return action.get();
        }
        return nullptr;
    }
};

std::vector<std::unique_ptr<Registry>> g_registries;

Registry *registryFor(lua_State *L)
{
    LuaInstance *instance = LuaInstance::fromState(L);
    if (!instance) return nullptr;
    for (auto &registry : g_registries)
    {
        if (registry->instance == instance) return registry.get();
    }
    return nullptr;
}

std::string trim(std::string value)
{
    const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

// Splits an action path or filter into segments around '>'.
std::vector<std::string> split_segments(const std::string &text)
{
    std::vector<std::string> segments;
    std::string current;
    for (char c : text)
    {
        if (c == '>')
        {
            segments.push_back(trim(current));
            current.clear();
        }
        else
        {
            current.push_back(c);
        }
    }
    segments.push_back(trim(current));
    return segments;
}

// Filters are "Category[] > (Name | *)"; "*" matches every remaining segment.
bool matches_filter(const std::string &path, const std::string &filter)
{
    const std::vector<std::string> filterParts = split_segments(filter);
    const std::vector<std::string> pathParts = split_segments(path);
    for (size_t i = 0; i < filterParts.size(); ++i)
    {
        if (filterParts[i] == "*") return true;
        if (i >= pathParts.size() || filterParts[i] != pathParts[i]) return false;
    }
    return filterParts.size() == pathParts.size();
}

bool table_opt_bool(lua_State *L, int table, const char *name, bool fallback)
{
    if (lua_getfield(L, table, name) == LUA_TNIL)
    {
        lua_pop(L, 1);
        return fallback;
    }
    const bool value = lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);
    return value;
}

// Stores the function found at table.field as a reference, or LUA_NOREF.
int table_opt_ref(lua_State *L, int table, const char *name)
{
    if (lua_getfield(L, table, name) != LUA_TFUNCTION)
    {
        lua_pop(L, 1);
        return LUA_NOREF;
    }
    const int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    return ref;
}

// Reads a Hotkey or LegacyHotkey table; returns false only for invalid shapes.
bool read_hotkey(lua_State *L, int index, Hotkey &out)
{
    luaL_checktype(L, index, LUA_TTABLE);
    out = {};
    out.ctrl = table_opt_bool(L, index, "ctrl", false);
    out.shift = table_opt_bool(L, index, "shift", false);
    out.alt = table_opt_bool(L, index, "alt", false);

    if (lua_getfield(L, index, "key") != LUA_TNIL)
    {
        out.valid = true;
        out.isSdl = false;
        out.value = static_cast<int>(luaL_checkinteger(L, -1));
        lua_pop(L, 1);
        return true;
    }
    lua_pop(L, 1);

    if (lua_getfield(L, index, "trigger") != LUA_TNIL)
    {
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            luaL_error(L, "hotkey trigger must be a table");
            return false;
        }
        lua_getfield(L, -1, "type");
        const char *type = lua_tostring(L, -1);
        if (type && std::string(type) == "keycode")
        {
            lua_getfield(L, -2, "value");
            out.valid = true;
            out.isSdl = true;
            out.value = static_cast<int>(luaL_checkinteger(L, -1));
            lua_pop(L, 1);
        }
        else if (!type || std::string(type) != "none")
        {
            lua_pop(L, 1);
            lua_pop(L, 1);
            luaL_error(L, "unsupported hotkey trigger type");
            return false;
        }
        lua_pop(L, 1); // type field
        lua_pop(L, 1); // trigger table
        return true;
    }
    lua_pop(L, 1);
    return true; // no key: a modifier-less hotkey that only shows in listings
}

bool hotkey_matches(const Hotkey &hotkey, int vk, int sdl, bool ctrl, bool alt, bool shift)
{
    if (!hotkey.valid) return false;
    if (hotkey.isSdl ? sdl != hotkey.value : vk != hotkey.value) return false;
    return hotkey.ctrl == ctrl && hotkey.shift == shift && hotkey.alt == alt;
}

// Calls a stored callback with no arguments (used for on_release).
void call_action_callback(lua_State *L, Registry &registry, Action &action, int ref)
{
    if (ref == LUA_NOREF) return;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        const char *err = lua_tostring(L, -1);
        registry.instance->appendLog(QString("[Error action '%1'] %2\n").arg(QString::fromStdString(action.path),
            err ? QString::fromUtf8(err) : QString("action callback failed")));
        lua_pop(L, 1);
    }
}

// invoke() shared between the Lua function and hotkey dispatch.
bool invoke_action(lua_State *L, Registry &registry, Action &action, bool up, bool releaseOnRepress, int paramsIndex)
{
    if (up)
    {
        if (action.onRelease != LUA_NOREF) call_action_callback(L, registry, action, action.onRelease);
        action.pressed = false;
        return true;
    }
    if (action.pressed && releaseOnRepress && action.onRelease != LUA_NOREF)
    {
        call_action_callback(L, registry, action, action.onRelease);
    }
    if (action.onPress != LUA_NOREF)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, action.onPress);
        if (lua_istable(L, paramsIndex)) lua_pushvalue(L, paramsIndex);
        else lua_newtable(L);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK)
        {
            const char *err = lua_tostring(L, -1);
            registry.instance->appendLog(QString("[Error action '%1'] %2\n").arg(QString::fromStdString(action.path),
                err ? QString::fromUtf8(err) : QString("action callback failed")));
            lua_pop(L, 1);
        }
    }
    action.pressed = true;
    return true;
}

bool call_predicate(lua_State *L, int ref, bool fallback)
{
    if (ref == LUA_NOREF) return fallback;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    if (lua_pcall(L, 0, 1, 0) != LUA_OK)
    {
        lua_pop(L, 2);
        return fallback;
    }
    const bool value = lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);
    return value;
}

void push_path_array(lua_State *L, const std::vector<std::string> &paths)
{
    lua_createtable(L, static_cast<int>(paths.size()), 0);
    int index = 1;
    for (const auto &path : paths)
    {
        lua_pushstring(L, path.c_str());
        lua_rawseti(L, -2, index++);
    }
}

// ---------------------------------------------------------------- action.* functions

int action_add(lua_State *L)
{
    Registry *registry = registryFor(L);
    if (!registry) return luaL_error(L, "no action registry");
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "path");
    const std::string path = luaL_checkstring(L, -1);
    lua_pop(L, 1);

    if (registry->find(path))
    {
        lua_pushboolean(L, false);
        return 1;
    }

    auto action = std::make_unique<Action>();
    action->path = path;
    action->onPress = table_opt_ref(L, 1, "on_press");
    action->onRelease = table_opt_ref(L, 1, "on_release");
    action->getActive = table_opt_ref(L, 1, "get_active");
    action->getEnabled = table_opt_ref(L, 1, "get_enabled");
    registry->actions.push_back(std::move(action));
    lua_pushboolean(L, true);
    return 1;
}

int action_remove(lua_State *L)
{
    Registry *registry = registryFor(L);
    if (!registry) return luaL_error(L, "no action registry");
    const std::string filter = luaL_checkstring(L, 1);

    std::vector<std::string> removed;
    for (auto it = registry->actions.begin(); it != registry->actions.end();)
    {
        if (!matches_filter((*it)->path, filter))
        {
            ++it;
            continue;
        }
        removed.push_back((*it)->path);
        for (int ref : {(*it)->onPress, (*it)->onRelease, (*it)->getActive, (*it)->getEnabled})
        {
            if (ref != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, ref);
        }
        it = registry->actions.erase(it);
    }
    push_path_array(L, removed);
    return 1;
}

int action_associate_hotkey(lua_State *L)
{
    Registry *registry = registryFor(L);
    if (!registry) return luaL_error(L, "no action registry");
    const std::string path = luaL_checkstring(L, 1);
    Action *action = registry->find(path);
    if (!action)
    {
        lua_pushboolean(L, false);
        return 1;
    }
    Hotkey hotkey;
    if (!read_hotkey(L, 2, hotkey)) return 0;
    const bool overwrite = lua_isnoneornil(L, 3) ? true : lua_toboolean(L, 3) != 0;
    if (!overwrite && action->hotkey.valid && hotkey.valid)
    {
        lua_pushboolean(L, false);
        return 1;
    }
    action->hotkey = hotkey;
    lua_pushboolean(L, true);
    return 1;
}

int action_begin_batch_work(lua_State *)
{
    // no action UI to batch updates for
    return 0;
}

int action_end_batch_work(lua_State *)
{
    return 0;
}

int action_notify_changed(lua_State *L)
{
    (void)L;
    return 0;
}

int action_get_display_name(lua_State *L)
{
    Registry *registry = registryFor(L);
    if (!registry) return luaL_error(L, "no action registry");
    const std::string filter = luaL_checkstring(L, 1);
    for (auto &action : registry->actions)
    {
        if (!matches_filter(action->path, filter)) continue;
        const std::vector<std::string> segments = split_segments(action->path);
        // the leading '#' marks hidden actions; keep it out of display names
        std::string name = segments.back();
        if (!name.empty() && name.front() == '#') name.erase(0, 1);
        lua_pushstring(L, name.c_str());
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

int action_get_enabled(lua_State *L)
{
    Registry *registry = registryFor(L);
    if (!registry) return luaL_error(L, "no action registry");
    Action *action = registry->find(luaL_checkstring(L, 1));
    if (!action)
    {
        lua_pushnil(L);
        return 1;
    }
    lua_pushboolean(L, call_predicate(L, action->getEnabled, true));
    return 1;
}

int action_get_active(lua_State *L)
{
    Registry *registry = registryFor(L);
    if (!registry) return luaL_error(L, "no action registry");
    Action *action = registry->find(luaL_checkstring(L, 1));
    if (!action)
    {
        lua_pushnil(L);
        return 1;
    }
    lua_pushboolean(L, call_predicate(L, action->getActive, false));
    return 1;
}

int action_get_activatability(lua_State *L)
{
    Registry *registry = registryFor(L);
    if (!registry) return luaL_error(L, "no action registry");
    Action *action = registry->find(luaL_checkstring(L, 1));
    if (!action)
    {
        lua_pushnil(L);
        return 1;
    }
    // no activatability hook in the contract: report whether the action is enabled
    lua_pushboolean(L, call_predicate(L, action->getEnabled, true));
    return 1;
}

int action_get_params(lua_State *L)
{
    Registry *registry = registryFor(L);
    if (!registry) return luaL_error(L, "no action registry");
    Action *action = registry->find(luaL_checkstring(L, 1));
    if (!action)
    {
        lua_pushnil(L);
        return 1;
    }
    lua_createtable(L, 0, 1);
    lua_pushstring(L, action->path.c_str());
    lua_setfield(L, -2, "path");
    return 1;
}

int action_get_paths_matching_filter(lua_State *L)
{
    Registry *registry = registryFor(L);
    if (!registry) return luaL_error(L, "no action registry");
    const std::string filter = luaL_checkstring(L, 1);
    std::vector<std::string> matches;
    for (auto &action : registry->actions)
    {
        if (matches_filter(action->path, filter)) matches.push_back(action->path);
    }
    push_path_array(L, matches);
    return 1;
}

int action_invoke(lua_State *L)
{
    Registry *registry = registryFor(L);
    if (!registry) return luaL_error(L, "no action registry");
    Action *action = registry->find(luaL_checkstring(L, 1));
    if (!action)
    {
        lua_pushboolean(L, false);
        return 1;
    }
    const bool up = lua_toboolean(L, 2) != 0;
    const bool releaseOnRepress = lua_isnoneornil(L, 3) ? true : lua_toboolean(L, 3) != 0;
    lua_pushboolean(L, invoke_action(L, *registry, *action, up, releaseOnRepress, 4));
    return 1;
}

int action_lock_hotkeys(lua_State *L)
{
    Registry *registry = registryFor(L);
    if (!registry) return luaL_error(L, "no action registry");
    registry->hotkeysLocked = lua_isnoneornil(L, 1) ? true : lua_toboolean(L, 1) != 0;
    if (registry->lockedHotkeysRef != LUA_NOREF)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, registry->lockedHotkeysRef);
        registry->lockedHotkeysRef = LUA_NOREF;
    }
    if (lua_istable(L, 2))
    {
        lua_pushvalue(L, 2);
        registry->lockedHotkeysRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    return 0;
}

int action_get_locked_hotkeys(lua_State *L)
{
    Registry *registry = registryFor(L);
    if (!registry) return luaL_error(L, "no action registry");
    if (!registry->hotkeysLocked)
    {
        lua_pushnil(L);
        return 1;
    }
    if (registry->lockedHotkeysRef == LUA_NOREF)
    {
        lua_createtable(L, 0, 0); // empty: every hotkey is locked
        return 1;
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, registry->lockedHotkeysRef);
    return 1;
}

int action_get_hotkeys_locked(lua_State *L)
{
    Registry *registry = registryFor(L);
    if (!registry) return luaL_error(L, "no action registry");
    lua_pushboolean(L, registry->hotkeysLocked);
    return 1;
}

} // namespace

namespace LuaAction
{
void registerInstance(lua_State *L, LuaInstance *instance)
{
    auto registry = std::make_unique<Registry>();
    registry->instance = instance;
    g_registries.push_back(std::move(registry));

    static const luaL_Reg functions[] = {
        {"add", action_add},
        {"remove", action_remove},
        {"associate_hotkey", action_associate_hotkey},
        {"begin_batch_work", action_begin_batch_work},
        {"end_batch_work", action_end_batch_work},
        {"notify_display_name_changed", action_notify_changed},
        {"notify_enabled_changed", action_notify_changed},
        {"notify_active_changed", action_notify_changed},
        {"get_display_name", action_get_display_name},
        {"get_enabled", action_get_enabled},
        {"get_active", action_get_active},
        {"get_activatability", action_get_activatability},
        {"get_params", action_get_params},
        {"get_actions_matching_filter", action_get_paths_matching_filter},
        {"invoke", action_invoke},
        {"lock_hotkeys", action_lock_hotkeys},
        {"get_locked_hotkeys", action_get_locked_hotkeys},
        {"get_hotkeys_locked", action_get_hotkeys_locked},
        {nullptr, nullptr},
    };
    luaL_newlib(L, functions);
    lua_setglobal(L, "action");
}

void unregisterInstance(LuaInstance *instance)
{
    std::erase_if(g_registries, [instance](const std::unique_ptr<Registry> &registry) {
        return registry->instance == instance;
    });
}

bool dispatchKey(LuaInstance *instance, int vk, int sdl, bool pressed, bool ctrl, bool alt, bool shift, bool repeat)
{
    if (!instance || repeat) return false;
    for (auto &reg : g_registries)
    {
        if (reg->instance != instance) continue;
        lua_State *L = instance->state();
        if (!L) return false;
        for (auto &action : reg->actions)
        {
            if (!hotkey_matches(action->hotkey, vk, sdl, ctrl, alt, shift)) continue;
            if (reg->hotkeysLocked) return false; // locked: let scripts see the raw key
            invoke_action(L, *reg, *action, !pressed, true, 0);
            return true;
        }
    }
    return false;
}

} // namespace LuaAction
