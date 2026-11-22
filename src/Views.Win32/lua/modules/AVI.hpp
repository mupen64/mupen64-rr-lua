/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <CaptureManager.hpp>
#include <lua/LuaRealmManager.hpp>

namespace LuaCore::Avi
{
static int start(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    const auto callback = LuaRealmManager::completion_callback(L, 2);
    CaptureManager::start_capture(path, static_cast<Config::EncoderType>(g_config.encoder_type), false, callback);
    return 0;
}

static int stop(lua_State *L)
{
    CaptureManager::stop_capture(LuaRealmManager::completion_callback(L, 1));
    return 0;
}

static int capturing(lua_State *L)
{
    lua_pushboolean(L, CaptureManager::is_capturing());
    return 1;
}
} // namespace LuaCore::Avi
