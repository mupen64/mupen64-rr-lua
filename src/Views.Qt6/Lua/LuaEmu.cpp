/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaInstance.hpp"

#include <cstring>
#include <string>

#include <QColor>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>

#include <lua.hpp>

#include "LuaInternal.hpp"
#include "EmuContext.hpp"

namespace LuaInternal
{

namespace
{

int register_callback_helper(lua_State *L, LuaCallbackKey key)
{
    auto *inst = getInstance(L);
    if (!inst) return 0;

    if (!lua_isfunction(L, 1))
    {
        return luaL_error(L, "callback must be a function");
    }

    bool unregister = lua_toboolean(L, 2);
    if (unregister)
    {
        inst->unregisterCallback(key, 1);
    }
    else
    {
        inst->registerCallback(key, 1);
    }
    return 0;
}

int emu_atstop(lua_State *L)
{
    return register_callback_helper(L, LuaCallbackKey::Stop);
}
int emu_framecount(lua_State *L)
{
    auto *ctx = EmuContext::rawContext();
    if (ctx && ctx->vcr_get_current_vi)
    {
        lua_pushinteger(L, ctx->vcr_get_current_vi());
    }
    else
    {
        lua_pushinteger(L, 0);
    }
    return 1;
}

int emu_samplecount(lua_State *L)
{
    auto *ctx = EmuContext::rawContext();
    if (ctx && ctx->vcr_get_seek_info)
    {
        lua_pushinteger(L, ctx->vcr_get_seek_info().current_sample);
    }
    else
    {
        lua_pushinteger(L, 0);
    }
    return 1;
}

int emu_pause(lua_State *L)
{
    if (auto *ctx = EmuContext::instance())
    {
        bool p = lua_toboolean(L, 1);
        ctx->setPaused(p);
    }
    return 0;
}

int emu_getpause(lua_State *L)
{
    if (auto *ctx = EmuContext::instance())
    {
        lua_pushboolean(L, ctx->isPaused());
    }
    else
    {
        lua_pushboolean(L, false);
    }
    return 1;
}

int emu_speed(lua_State *L)
{
    if (auto *ctx = EmuContext::instance())
    {
        int spd = static_cast<int>(luaL_checkinteger(L, 1));
        ctx->setSpeedModifier(spd);
    }
    return 0;
}

int emu_getspeed(lua_State *L)
{
    if (auto *ctx = EmuContext::instance())
    {
        lua_pushinteger(L, ctx->speedModifier());
    }
    else
    {
        lua_pushinteger(L, 100);
    }
    return 1;
}

int emu_get_hint(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst) return 0;
    const char *key = luaL_checkstring(L, 1);
    std::string val = inst->queryHint(key);
    lua_pushstring(L, val.c_str());
    return 1;
}

int emu_set_hint(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst) return 0;
    const char *key = luaL_checkstring(L, 1);
    const char *val = luaL_optstring(L, 2, "");
    inst->setHint(key, val);
    return 0;
}

int emu_get_pref(lua_State *L)
{
    const char *pref = luaL_checkstring(L, 1);
    if (std::strcmp(pref, "theme") == 0)
    {
        bool isDark = qGuiApp ? (qGuiApp->palette().color(QPalette::Window).lightness() < 128) : true;
        lua_pushstring(L, isDark ? "dark" : "light");
        return 1;
    }
    else if (std::strcmp(pref, "accent_color") == 0)
    {
        QColor accent = qGuiApp ? qGuiApp->palette().color(QPalette::Highlight) : QColor(0, 120, 215);
        lua_createtable(L, 0, 4);
        lua_pushnumber(L, accent.redF());
        lua_setfield(L, -2, "r");
        lua_pushnumber(L, accent.greenF());
        lua_setfield(L, -2, "g");
        lua_pushnumber(L, accent.blueF());
        lua_setfield(L, -2, "b");
        lua_pushnumber(L, accent.alphaF());
        lua_setfield(L, -2, "a");
        return 1;
    }
    else if (std::strcmp(pref, "reduced_motion") == 0)
    {
        lua_pushboolean(L, false);
        return 1;
    }
    else if (std::strcmp(pref, "caret_blink_time") == 0)
    {
        lua_pushinteger(L, qGuiApp ? qGuiApp->styleHints()->cursorFlashTime() : 500);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

} // namespace

void registerEmuAPIs(lua_State *L)
{
    lua_newtable(L);

    const luaL_Reg emuFuncs[] = {
        {"atstop", emu_atstop},
        {"framecount", emu_framecount},
        {"samplecount", emu_samplecount},
        {"pause", emu_pause},
        {"getpause", emu_getpause},
        {"speed", emu_speed},
        {"getspeed", emu_getspeed},
        {"get_hint", emu_get_hint},
        {"set_hint", emu_set_hint},
        {"get_pref", emu_get_pref},
        {nullptr, nullptr},
    };

    luaL_setfuncs(L, emuFuncs, 0);
    lua_setglobal(L, "emu");
}

} // namespace LuaInternal
