/*
 * Copyright (c) 2026, TASInput maintainers, contributors, and original authors (nitsuja, Deflection).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma warning(push, 0)
#include <CommonPCH.h>
#include <VersionNameHelpers.h>
#include <core_api.h>
#include <Views.Win32/ViewPlugin.h>
#include <filesystem>
#include <string>
#include <format>
#include <algorithm>
#include <memory>
#include <functional>
#include <vector>
#include <memory>
#include <tchar.h>
#include <span>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <cstdio>
#include <map>
#include <cassert>
#include <cmath>
#include <cfloat>
#include <stack>
#include <numeric>
#include <variant>
#include <Windows.h>
#include <shlobj.h>
#include <commctrl.h>
#include <windowsx.h>
#include <shellscalingapi.h>
#include <core_plugin.h>
#include <Resource.h>
#include <gdiplus.h>
#include <SDL3/SDL.h>
#pragma warning(pop)

#include <Helpers.h>
