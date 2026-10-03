/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Common.hpp"
#include <lua/LuaEnvironment.hpp>

LuaEnvironment::LuaEnvironment(const std::filesystem::path &path, LuaDestroyFn destroying, LuaPrintFn print)
    : m_l(luaL_newstate()), path(path), destroying(std::move(destroying)), print(std::move(print))
{
    need(is_on_gui_thread(), "LuaEnvironment constructor must be called on the GUI thread");
}

LuaEnvironment::~LuaEnvironment()
{
    if (m_l)
    {
        lua_close(m_l);
        m_l = nullptr;
    }
}
