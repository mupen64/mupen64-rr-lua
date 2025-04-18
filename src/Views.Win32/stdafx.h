/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#define SPDLOG_LEVEL_NAMES {"🔍", "🪲", "ℹ️", "⚠️", "❌", "💥", ""}

#include <filesystem>
#include <string>
#include <format>
#include <algorithm>
#include <memory>
#include <functional>
#include <vector>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <span>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <intsafe.h>
#include <cstdio>
#include <stdint.h>
#include <map>
#include <cassert>
#include <math.h>
#include <float.h>
#include <stacktrace>
#include <stdarg.h>
#include <optional>
#include <string_view>
#include <variant>
#include <csetjmp>
#include <locale>
#include <cctype>
#include <any>
#include <stack>
#include <fstream>
#include <deque>
#include <numeric>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <spdlog/logger.h>
#include <microlru.h>
#include <Config.h>
#include <core_api.h>
#include <IOHelpers.h>
#include <MiscHelpers.h>
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
#include <lua/LuaConsole.h>
#include <strsafe.h>
#include <commdlg.h>