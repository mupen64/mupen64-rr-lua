/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LUAACTION_HPP
#define LUAACTION_HPP

#include <lua.hpp>

#include <vector>

class LuaInstance;

// Script-registered actions (action.* API), with the hotkey associations that the
// main window's key events dispatch to. This is the Lua-visible part of the action
// system: the Qt6 frontend has no action menus, so display names and batch updates
// are accepted but have no effect.
namespace LuaAction
{
// Registers the `action` global and the instance that owns it.
void registerInstance(lua_State *L, LuaInstance *instance);
void unregisterInstance(LuaInstance *instance);

// Matches key events (as Windows VKeycodes, like pushKeyTable) against the actions'
// hotkeys and invokes them. Returns true when an unlocked hotkey consumed the event.
bool dispatchKey(LuaInstance *instance, int vk, int sdl, bool pressed, bool ctrl, bool alt, bool shift, bool repeat);
} // namespace LuaAction

#endif
