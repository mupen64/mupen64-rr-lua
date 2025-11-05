/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <Messenger.h>
#include <Plugin.h>
#include <components/Statusbar.h>
#include <lua/LuaCallbacks.h>

namespace LuaCore::Emu
{
static int GetVICount(lua_State *L)
{
    lua_pushinteger(L, g_main_ctx.core_ctx->vcr_get_current_vi());
    return 1;
}

static int GetSampleCount(lua_State *L)
{
    const core_vcr_seek_info info = g_main_ctx.core_ctx->vcr_get_seek_info();
    lua_pushinteger(L, info.current_sample);
    return 1;
}

static int GetInputCount(lua_State *L)
{
    lua_pushinteger(L, g_input_count);
    return 1;
}

static int subscribe_atupdatescreen(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_ATUPDATESCREEN);
    return 0;
}

static int subscribe_atdrawd2d(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_ATDRAWD2D);
    return 0;
}

static int subscribe_atvi(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_ATVI);
    return 0;
}

static int subscribe_atinput(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_ATINPUT);
    return 0;
}

static int subscribe_atstop(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_ATSTOP);
    return 0;
}

static int subscribe_atwindowmessage(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_WINDOWMESSAGE);
    return 0;
}

static int subscribe_atinterval(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_ATINTERVAL);
    return 0;
}

static int subscribe_atplaymovie(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_ATPLAYMOVIE);
    return 0;
}

static int subscribe_atstopmovie(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_ATSTOPMOVIE);
    return 0;
}

static int subscribe_atloadstate(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_ATLOADSTATE);
    return 0;
}

static int subscribe_atsavestate(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_ATSAVESTATE);
    return 0;
}

static int subscribe_atreset(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_ATRESET);
    return 0;
}

static int subscribe_atseekcompleted(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_ATSEEKCOMPLETED);
    return 0;
}

static int subscribe_atwarpmodifystatuschanged(lua_State *L)
{
    LuaCallbacks::register_or_unregister_function(L, LuaCallbacks::REG_ATWARPMODIFYSTATUSCHANGED);
    return 0;
}

static int Screenshot(lua_State *L)
{
    g_plugin_funcs.video_capture_screen((char *)luaL_checkstring(L, 1));
    return 0;
}

static int IsMainWindowInForeground(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    lua_pushboolean(L, GetForegroundWindow() == g_main_ctx.hwnd || GetActiveWindow() == g_main_ctx.hwnd);
    return 1;
}

static int LuaPlaySound(lua_State *L)
{
    PlaySound(IOUtils::to_wide_string(luaL_checkstring(L, 1)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
    return 1;
}

static int EmuPause(lua_State *L)
{
    if (!lua_toboolean(L, 1))
    {
        g_main_ctx.core_ctx->vr_pause_emu();
    }
    else
    {
        g_main_ctx.core_ctx->vr_resume_emu();
    }
    return 0;
}

static int GetEmuPause(lua_State *L)
{
    lua_pushboolean(L, g_main_ctx.core_ctx->vr_get_paused());
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
    g_main_ctx.core_ctx->vr_on_speed_modifier_changed();
    return 0;
}

static int GetFastForward(lua_State *L)
{
    lua_pushboolean(L, g_main_ctx.fast_forward);
    return 1;
}

static int SetFastForward(lua_State *L)
{
    g_main_ctx.fast_forward = lua_toboolean(L, 1);
    Messenger::broadcast(Messenger::Message::FastForwardNeedsUpdate, nullptr);
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
#define B(x, n) {x, n}
    const NameAndVariable list[] = {A("rdram", g_main_ctx.core_ctx->rdram),
                                    A("rdram_register", g_main_ctx.core_ctx->rdram_register),
                                    A("MI_register", g_main_ctx.core_ctx->MI_register),
                                    A("pi_register", g_main_ctx.core_ctx->pi_register),
                                    A("sp_register", g_main_ctx.core_ctx->sp_register),
                                    A("rsp_register", g_main_ctx.core_ctx->rsp_register),
                                    A("si_register", g_main_ctx.core_ctx->si_register),
                                    A("vi_register", g_main_ctx.core_ctx->vi_register),
                                    A("ri_register", g_main_ctx.core_ctx->ri_register),
                                    A("ai_register", g_main_ctx.core_ctx->ai_register),
                                    A("dpc_register", g_main_ctx.core_ctx->dpc_register),
                                    A("dps_register", g_main_ctx.core_ctx->dps_register),
                                    B("SP_DMEM", g_main_ctx.core_ctx->SP_DMEM),
                                    B("PIF_RAM", g_main_ctx.core_ctx->PIF_RAM),
                                    {NULL, NULL}};
#undef A
#undef B
    const char *s = lua_tostring(L, 1);
    for (const NameAndVariable *p = list; p->name; p++)
    {
        if (lstrcmpiA(p->name, s) == 0)
        {
            lua_pushinteger(L, (unsigned)p->pointer);
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

    std::wstring version = get_mupen_name(true);

    if (type > 0)
    {
        version = version.substr(std::string("Mupen 64 ").size());
    }

    lua_pushstring(L, IOUtils::to_utf8_string(version).c_str());
    return 1;
}

// emu
static int ConsoleWriteLua(lua_State *L)
{
    auto lua = LuaManager::get_environment_for_state(L);
    const auto str = IOUtils::to_wide_string(luaL_checkstring(L, 1));

    lua->print(lua, str + L"\r\n");
    return 0;
}

static int StatusbarWrite(lua_State *L)
{
    Statusbar::post(IOUtils::to_wide_string(lua_tostring(L, 1)));
    return 0;
}
} // namespace LuaCore::Emu
