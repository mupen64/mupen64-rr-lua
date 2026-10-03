/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <lua/LuaManager.hpp>

CoreButtons g_new_controller_data[4]{};
bool g_overwrite_controller_data[4]{};
size_t g_input_count{};
