/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <lua/LuaRenderer.hpp>
#include <lua.h>

class LuaRealm;

class LuaWindow
{
  public:
    LuaRealm *realm{};
    HWND hwnd{};
    std::string id;
    std::string anchor;
    std::unique_ptr<LuaRenderer> renderer;
    bool closed{};
    bool closable{true};
    bool updating_anchor{};

    void reposition();
    void move_group();
    bool close();
    bool call(uint8_t key, const std::function<int(lua_State *)> &invoke);
    static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

void handle_lua_window_events(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace LuaCore::Window
{
void register_types(lua_State *L);
int create(lua_State *L);
int main(lua_State *L);
} // namespace LuaCore::Window
