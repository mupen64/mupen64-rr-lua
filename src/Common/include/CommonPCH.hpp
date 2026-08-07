/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#pragma warning(push, 0)
#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <bit>
#include <concepts>
#include <charconv>
#include <cassert>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <csetjmp>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <initializer_list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <numeric>
#include <queue>
#include <ranges>
#include <span>
#include <stack>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <variant>
#include <vector>
#include <latch>
#if !defined(_WIN32)
// Implementation of C11 Annex K for Linux
#include <safe_str_lib.h>
#endif

#include <xxh64.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_audio.h>

#include "MiscHelpers.hpp"
#include "StrUtils.hpp"
#include "IOUtils.hpp"
#pragma warning(pop)
