/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#ifdef _MSC_VER
#define _MSVC_STL_HARDENING 1
#endif

#define SPDLOG_LEVEL_NAMES {"🔍", "🪲", "ℹ️", "⚠️", "❌", "💥", ""}


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
#include <strsafe.h>
#include <commdlg.h>
#include <cwctype>
#include <WinDarkMode.h>
using Microsoft::WRL::ComPtr;
#pragma warning(pop)

#include <Loggers.hpp>
#include <Common.Win32/WinUtils.hpp>
#include <Main.hpp>
#include <lua/LuaHelpers.hpp>
#include <lua/LuaTypes.hpp>
#include <Common.Views/Config.hpp>
#include <Common.Win32/ResizeAnchor.hpp>
#include <Common.Win32/JoystickControl.hpp>
#include <Common/VersionNameHelpers.hpp>
#include <Common.Views/IDialogService.hpp>
#include <Common.Views/App.hpp>

// Workaround for broken LVN_GETDISPINFO under MinGW
// TODO: Remove when fixed
inline void copy_listview_text(LPSTR destination, int capacity, const std::string &text)
{
    if (capacity <= 0) return;
    strncpy(destination, text.c_str(), capacity);
    destination[capacity - 1] = '\0';
}

// Workaround for broken LVN_GETDISPINFO under MinGW
// TODO: Remove when fixed
inline void copy_listview_text(LPWSTR destination, int capacity, const std::string &text)
{
    if (capacity <= 0) return;
    const auto wide_text = IOUtils::to_wide_string(text);
    wcsncpy(destination, wide_text.c_str(), capacity);
    destination[capacity - 1] = L'\0';
}
