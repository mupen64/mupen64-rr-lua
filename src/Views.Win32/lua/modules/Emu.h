/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <gui/Main.h>
#include <gui/features/Statusbar.h>
#include <lua/LuaConsole.h>

namespace LuaCore::Emu
{
    static int GetVICount(lua_State* L)
    {
        lua_pushinteger(L, core_vcr_get_current_vi());
        return 1;
    }

    static int GetSampleCount(lua_State* L)
    {
        std::pair<size_t, size_t> pair{};
        core_vcr_get_seek_completion(pair);
        lua_pushinteger(L, pair.first);
        return 1;
    }

    static int GetInputCount(lua_State* L)
    {
        lua_pushinteger(L, inputCount);
        return 1;
    }

    static int RegisterUpdateScreen(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_ATUPDATESCREEN);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_ATUPDATESCREEN);
        }
        return 0;
    }

    static int RegisterAtDrawD2D(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_ATDRAWD2D);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_ATDRAWD2D);
        }
        return 0;
    }

    static int RegisterVI(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_ATVI);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_ATVI);
        }
        return 0;
    }

    static int RegisterInput(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_ATINPUT);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_ATINPUT);
        }
        return 0;
    }

    static int RegisterStop(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_ATSTOP);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_ATSTOP);
        }
        return 0;
    }

    static int RegisterWindowMessage(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_WINDOWMESSAGE);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_WINDOWMESSAGE);
        }
        return 0;
    }

    static int RegisterInterval(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_ATINTERVAL);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_ATINTERVAL);
        }
        return 0;
    }

    static int RegisterPlayMovie(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_ATPLAYMOVIE);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_ATPLAYMOVIE);
        }
        return 0;
    }

    static int RegisterStopMovie(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_ATSTOPMOVIE);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_ATSTOPMOVIE);
        }
        return 0;
    }

    static int RegisterLoadState(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_ATLOADSTATE);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_ATLOADSTATE);
        }
        return 0;
    }

    static int RegisterSaveState(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_ATSAVESTATE);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_ATSAVESTATE);
        }
        return 0;
    }

    static int RegisterReset(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_ATRESET);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_ATRESET);
        }
        return 0;
    }

    static int RegisterSeekCompleted(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_ATSEEKCOMPLETED);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_ATSEEKCOMPLETED);
        }
        return 0;
    }

    static int RegisterWarpModifyStatusChanged(lua_State* L)
    {
        if (lua_toboolean(L, 2))
        {
            lua_pop(L, 1);
            UnregisterFunction(L, REG_ATWARPMODIFYSTATUSCHANGED);
        }
        else
        {
            if (lua_gettop(L) == 2)
                lua_pop(L, 1);
            RegisterFunction(L, REG_ATWARPMODIFYSTATUSCHANGED);
        }
        return 0;
    }

    static int Screenshot(lua_State* L)
    {
        g_core.plugin_funcs.video_capture_screen((char*)luaL_checkstring(L, 1));
        return 0;
    }

    static int IsMainWindowInForeground(lua_State* L)
    {
        LuaEnvironment* lua = get_lua_class(L);
        lua_pushboolean(
            L, GetForegroundWindow() == g_main_hwnd || GetActiveWindow() ==
            g_main_hwnd);
        return 1;
    }

    static int LuaPlaySound(lua_State* L)
    {
        PlaySound(string_to_wstring(luaL_checkstring(L, 1)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
        return 1;
    }

    static int SetGFX(lua_State* L)
    {
        // stub for now
        return 0;
    }

    static int EmuPause(lua_State* L)
    {
        if (!lua_toboolean(L, 1))
        {
            core_vr_pause_emu();
        }
        else
        {
            core_vr_resume_emu();
        }
        return 0;
    }

    static int GetEmuPause(lua_State* L)
    {
        lua_pushboolean(L, core_vr_get_paused());
        return 1;
    }

    static int GetSpeed(lua_State* L)
    {
        lua_pushinteger(L, g_config.core.fps_modifier);
        return 1;
    }

    static int SetSpeed(lua_State* L)
    {
        g_config.core.fps_modifier = luaL_checkinteger(L, 1);
        core_vr_on_speed_modifier_changed();
        return 0;
    }

    static int GetFastForward(lua_State* L)
    {
        lua_pushboolean(L, g_fast_forward);
        return 1;
    }

    static int SetFastForward(lua_State* L)
    {
        g_fast_forward = lua_toboolean(L, 1);
        Messenger::broadcast(Messenger::Message::FastForwardNeedsUpdate, nullptr);
        return 0;
    }

    static int SetSpeedMode(lua_State* L)
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

    static int GetAddress(lua_State* L)
    {
        struct NameAndVariable
        {
            const char* name;
            void* pointer;
        };
#define A(n) {#n, &n}
#define B(n) {#n, n}
        const NameAndVariable list[] = {
            A(g_core.rdram),
            A(g_core.rdram_register),
            A(g_core.MI_register),
            A(g_core.pi_register),
            A(g_core.sp_register),
            A(g_core.rsp_register),
            A(g_core.si_register),
            A(g_core.vi_register),
            A(g_core.ri_register),
            A(g_core.ai_register),
            A(g_core.dpc_register),
            A(g_core.dps_register),
            B(g_core.SP_DMEM),
            B(g_core.PIF_RAM),
            {NULL, NULL}
        };
#undef A
#undef B
        const char* s = lua_tostring(L, 1);
        for (const NameAndVariable* p = list; p->name; p++)
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

    static int GetMupenVersion(lua_State* L)
    {
        int type = luaL_optnumber(L, 1, 0);

    	// 0 = name + version number
    	// 1 = version number

    	std::wstring version = get_mupen_name();

        if (type > 0)
        {
	        version = version.substr(std::string("Mupen 64 ").size());
        }

        lua_pushstring(L, wstring_to_string(version).c_str());
        return 1;
    }

    //emu
    static int ConsoleWriteLua(lua_State* L)
    {
        get_lua_class(L)->print(string_to_wstring(lua_tostring(L, 1)));
        return 0;
    }

    static int StatusbarWrite(lua_State* L)
    {
        Statusbar::post(string_to_wstring(lua_tostring(L, 1)));
        return 0;
    }
}
