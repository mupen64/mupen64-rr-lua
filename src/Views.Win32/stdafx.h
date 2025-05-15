/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#define SPDLOG_LEVEL_NAMES {"🔍", "🪲", "ℹ️", "⚠️", "❌", "💥", ""}

#include <Core/stdafx.h>
#include <core_api.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <spdlog/logger.h>
#include <microlru.h>
#include <Windows.h>
#include <commctrl.h>
#include <resource.h>
#include <ShlObj.h>
#include <DbgHelp.h>
#include <d2d1.h>
#include <dwrite.h>
#include <Shlwapi.h>
#include <shellapi.h>
#include <windowsx.h>
#include <Psapi.h>
#include <mmsystem.h>
#include <wincodec.h>
#include <gdiplus.h>
#include <d2d1.h>
#include <d2d1_3.h>
#include <d2d1helper.h>
#include <d2d1helper.h>
#include <d3d11.h>
#include <dcomp.h>
#include <shlobj_core.h>
#include <xxh64.h>
#include <strsafe.h>
#include <commdlg.h>
#include <unordered_set>
#include <stacktrace>

#include <Loggers.h>
#include <MiscHelpers.h>
#include <Main.h>
#include <lua/LuaTypes.h>
#include <Config.h>
#include <IOHelpers.h>
