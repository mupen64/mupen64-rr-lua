/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace LuaCore::Hotkey
{
    static int prompt(lua_State* L)
    {
        // TODO: Implement
        lua_pushboolean(L, 1);
        return 1;
    }
} // namespace LuaCore::Hotkey
