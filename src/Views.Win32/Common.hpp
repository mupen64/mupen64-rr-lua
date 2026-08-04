/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#ifdef _MSC_VER
#define _MSVC_STL_HARDENING 1
#endif

#define SPDLOG_LEVEL_NAMES                                                                                             \
    {                                                                                                                  \
        "🔍", "🪲", "ℹ️", "⚠️", "❌", "💥", ""                                                                           \
    }

#include <CommonPCH.hpp>
#include <m64rr/API.hpp>

#pragma warning(push, 0)
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <spdlog/logger.h>
#include <microlru.h>
#include <windows.h>
#include <commctrl.h>
#include <resource.h>
#include <shlobj.h>
#include <dbghelp.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <windowsx.h>
#include <psapi.h>
#include <mmsystem.h>
#include <wincodec.h>
#include <gdiplus.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <d2d1.h>
#include <d2d1_3.h>
#include <d2d1helper.h>
#include <d2d1effects.h>
#include <d3d11.h>
#include <dcomp.h>
#include <dwrite.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <dwmapi.h>
#include <xxh64.h>
#include <strsafe.h>
#include <commdlg.h>
#include <unordered_set>
#include <expected>
#include <ranges>
#include <set>
#include <cwctype>
#include <WinDarkMode.h>
using Microsoft::WRL::ComPtr;
#pragma warning(pop)

#include <Loggers.hpp>
#include <ViewHelpers.hpp>
#include <Main.hpp>
#include <lua/LuaHelpers.hpp>
#include <lua/LuaTypes.hpp>
#include <Config.hpp>
#include <ResizeAnchor.hpp>
#include <JoystickControl.hpp>
#include <VersionNameHelpers.hpp>
