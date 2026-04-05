--
-- Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

-- Start this script while in a level in SM64 and ensure that the breakpoint is hit.

dofile(debug.getinfo(1).source:sub(2):gsub("\\[^\\]+\\[^\\]+$", "") .. '\\test_prelude.lua')

local bp

bp = debugger.add_breakpoint(2150719408, function(state)
    print('breakpoint hit', state, debugger.disassemble(state))

    debugger.remove_breakpoint(bp)
end)
