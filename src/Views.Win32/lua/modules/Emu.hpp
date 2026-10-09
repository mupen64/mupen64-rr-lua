/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Common.Views/Messages.hpp>
#include <plugin/Plugin.hpp>
#include <components/Statusbar.hpp>
#include <lua/LuaRealmManager.hpp>

namespace LuaCore::Emu
{
static int get_hint(lua_State *L)
{
    const char *hint = luaL_checkstring(L, 1);
    auto *env = LuaManager::get_environment_for_state(L);

    if (!LUA_HINTS.contains(hint)) return luaL_error(L, "Unknown hint '%s'", hint);

    const auto value = env->query_hint(hint);
    lua_pushstring(L, value.c_str());
    return 1;
}

static int set_hint(lua_State *L)
{
    const char *hint = luaL_checkstring(L, 1);
    const char *value = luaL_checkstring(L, 2);

    auto *env = LuaManager::get_environment_for_state(L);
    const auto result = env->try_set_hint(hint, value);
    if (!result) return luaL_error(L, "%s", result.error().c_str());

    return 0;
}

static int GetVICount(lua_State *L)
{
    lua_pushinteger(L, g_main_ctx.CoreCtx->vcr_get_current_vi());
    return 1;
}

static int GetSampleCount(lua_State *L)
{
    const CoreVCRSeekInfo info = g_main_ctx.CoreCtx->vcr_get_seek_info();
    lua_pushinteger(L, info.current_sample);
    return 1;
}

static int GetInputCount(lua_State *L)
{
    lua_pushinteger(L, LuaRealmManager::instance().input_count);
    return 1;
}

static int register_callback(lua_State *L, const LuaRealmManager::callback_key key)
{
    auto *env = LuaRealmManager::instance().get_by_state(L);
    env->register_or_unregister_function(static_cast<uint8_t>(key));
    return 0;
}

static int subscribe_atupdatescreen(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATUPDATESCREEN);
}

static int subscribe_atpaint(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATPAINT);
}

static int subscribe_atvi(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATVI);
}

static int subscribe_atinput(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATINPUT);
}

static int subscribe_atstop(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATSTOP);
}

static int subscribe_atwindowmessage(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_WINDOWMESSAGE);
}

static int subscribe_atinterval(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATINTERVAL);
}

static int subscribe_atplaymovie(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATPLAYMOVIE);
}

static int subscribe_atstopmovie(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATSTOPMOVIE);
}

static int subscribe_atloadstate(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATLOADSTATE);
}

static int subscribe_atsavestate(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATSAVESTATE);
}

static int subscribe_atreset(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATRESET);
}

static int subscribe_atseekcompleted(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATSEEKCOMPLETED);
}

static int subscribe_atwarpmodifystatuschanged(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATWARPMODIFYSTATUSCHANGED);
}

static int subscribe_atkey(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATKEY);
}

static int subscribe_atmouse(lua_State *L)
{
    return register_callback(L, LuaRealmManager::REG_ATMOUSE);
}

static int Screenshot(lua_State *L)
{
    const auto path = luaL_checkstring(L, 1);
    PluginUtil::screenshot(path);
    return 0;
}

static int IsMainWindowInForeground(lua_State *L)
{
    auto lua = LuaRealmManager::instance().get_by_state(L);
    lua_pushboolean(L, GetForegroundWindow() == g_main_ctx.hwnd || GetActiveWindow() == g_main_ctx.hwnd);
    return 1;
}

static int LuaPlaySound(lua_State *L)
{
    PlaySound(luaL_checkstring(L, 1), NULL, SND_FILENAME | SND_ASYNC);
    return 1;
}

static int EmuPause(lua_State *L)
{
    // COMPAT: Inverted for compatibility with older scripts
    const auto pause = !lua_toboolean(L, 1);

    if (pause)
    {
        g_main_ctx.CoreCtx->vr_pause_emu();
    }
    else
    {
        g_main_ctx.CoreCtx->vr_resume_emu();
    }
    return 0;
}

static int GetEmuPause(lua_State *L)
{
    lua_pushboolean(L, g_main_ctx.CoreCtx->vr_get_paused());
    return 1;
}

static int GetSpeed(lua_State *L)
{
    lua_pushinteger(L, g_config.core.fps_modifier);
    return 1;
}

static int SetSpeed(lua_State *L)
{
    g_config.core.fps_modifier = luaL_checkinteger(L, 1);
    g_main_ctx.CoreCtx->vr_on_speed_modifier_changed();
    return 0;
}

static int get_speed_mode(lua_State *L)
{
    const auto mode = g_main_ctx.CoreCtx->vr_get_speed_mode();
    lua_pushinteger(L, (lua_Integer)mode);
    return 1;
}

static int set_speed_mode(lua_State *L)
{
    const auto mode = (CoreSpeedMode)luaL_checkinteger(L, 1);
    g_main_ctx.CoreCtx->vr_set_speed_mode(mode);
    return 0;
}

static int SetSpeedMode(lua_State *L)
{
    if (!strcmp(luaL_checkstring(L, 1), "normal"))
    {
        g_config.core.fps_modifier = 100;
    }
    else
    {
        g_config.core.fps_modifier = 10000;
    }
    return 0;
}

static int GetAddress(lua_State *L)
{
    struct NameAndVariable
    {
        const char *name;
        void *pointer;
    };
#define A(x, n) {x, &n}
#define B(x, n)                                                                                                        \
    {                                                                                                                  \
        x, n                                                                                                           \
    }
    const NameAndVariable list[] = {A("rdram", g_main_ctx.CoreCtx->rdram),
        A("rdram_register", g_main_ctx.CoreCtx->rdram_register), A("MI_register", g_main_ctx.CoreCtx->mi_register),
        A("pi_register", g_main_ctx.CoreCtx->pi_register), A("sp_register", g_main_ctx.CoreCtx->sp_register),
        A("rsp_register", g_main_ctx.CoreCtx->rsp_register), A("si_register", g_main_ctx.CoreCtx->si_register),
        A("vi_register", g_main_ctx.CoreCtx->vi_register), A("ri_register", g_main_ctx.CoreCtx->ri_register),
        A("ai_register", g_main_ctx.CoreCtx->ai_register), A("dpc_register", g_main_ctx.CoreCtx->dpc_register),
        A("dps_register", g_main_ctx.CoreCtx->dps_register), B("SP_DMEM", g_main_ctx.CoreCtx->sp_dmem),
        B("PIF_RAM", g_main_ctx.CoreCtx->pif_ram), {NULL, NULL}};
#undef A
#undef B
    const char *s = lua_tostring(L, 1);
    for (const NameAndVariable *p = list; p->name; p++)
    {
        if (lstrcmpiA(p->name, s) == 0)
        {
            lua_pushinteger(L, (lua_Integer)p->pointer);
            return 1;
        }
    }
    luaL_error(L, "Invalid variable name. (%s)", s);
    return 0;
}

static int GetMupenVersion(lua_State *L)
{
    int type = luaL_optnumber(L, 1, 0);

    // 0 = name + version number
    // 1 = version number

    std::string version = get_mupen_name(true);

    if (type > 0)
    {
        version = version.substr(std::string("Mupen 64 ").size());
    }

    lua_pushstring(L, version.c_str());
    return 1;
}

// emu
static int ConsoleWriteLua(lua_State *L)
{
    auto lua = LuaRealmManager::instance().get_by_state(L);
    const auto str = luaL_checkstring(L, 1);

    lua->print(lua, std::string(str) + "\r\n");
    return 0;
}

static int StatusbarWrite(lua_State *L)
{
    Statusbar::post(lua_tostring(L, 1));
    return 0;
}
} // namespace LuaCore::Emu
