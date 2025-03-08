/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
#include <deque>
#include <numeric>

#include <spdlog/logger.h>
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
// HACK: Microsoft deprecated multimedia timers with no real alternative and moved them to another header without updating the docs...
#include <mmsystem.h>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <luacode.h>
#include <luaconf.h>
}
#include <Luau/BytecodeBuilder.h>
#include <Luau/Compiler.h>